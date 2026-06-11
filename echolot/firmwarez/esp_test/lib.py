import sys
import time
import serial
import serial.tools.list_ports
import numpy as np
import matplotlib.pyplot as plt
import pyqtgraph as pg
from pyqtgraph.Qt import QtWidgets
from scipy.signal import butter, filtfilt

BAUDRATE = 115200
BAUDRATE = 921600

def getFilenamesFromArgs(default=""):
	if len(sys.argv) > 1:
		return sys.argv[1:] 
	if not default:
		exit("No files specified")
	return [default]

def serialGet(bytes, file=None):
	ports = [port for port, desc, hwid in sorted(serial.tools.list_ports.comports(), reverse=True)]
	print(f"Available ports: {ports}")

	if not len(ports):
		print("No ports found")
		exit()

	port = ports[0]
	if port == "COM1":
		print("No port found except COM1")
		exit()

	print(f"Connecting to {port}....")

	ser = serial.Serial(port, BAUDRATE, timeout=0)

	print("Connected! Reading")

	# buffer = bytearray()

	# raw = ser.read(bytes, timeout=0)

	# print("The tail is:",len(raw))

	# print("Reset the device")

	raw = bytearray()
	is_first = 1
	while 1:
		chunk = ser.read(bytes)
		print("	chunk: ",len(chunk))
		if not len(chunk) and not is_first:
			break
		is_first = 0
		raw += chunk
		time.sleep(1)

	print(f"Bytes received: ",len(raw))
	
	if file:
		with open(file,"wb") as f:
			f.write(raw)

	return raw

def readFiles(files):
	datas = []
	for file in files:
		with open(file,"rb") as f:
			datas.append(prepareRawData(f.read()))
	return datas

def displayFiles(files):
	displayData(readFiles(files))


def prepareRawData(raw):
	full_view = memoryview(raw)

	start = raw.find(bytes(">>DATA", encoding="utf-8"))
	stop  = raw.find(bytes(">>DATAEND", encoding="utf-8"))

	if start < 0:
		print("Начало данных куда-то проебали. Это очень хуёво...")
		exit()

	if stop < 0:
		print("Конец данных куда-то проебали. Да и хуй с ним!")
		# stop = (len(raw) >> 1) << 1

	if (stop-start)%2:
		stop -= 1

	return np.frombuffer(full_view[start+6:stop], dtype=np.uint16)


def displayData(datas):

	xsync = 0
	# start = data.

	# plt.figure(figsize=(14, 8))
	# plt.plot(data)

	# plt.grid(True)
	# plt.tight_layout()

	# plt.show()

	app = QtWidgets.QApplication([])

	win = pg.GraphicsLayoutWidget(show=True)

	plot1 = None
	for data in datas:
		plot = win.addPlot()
		if xsync:
			if not plot1:
				plot1 = plot
			else:
				plot.setXLink(plot1)

		plot.showGrid(x=True, y=True)
		curve = plot.plot(data)
		win.nextRow()

	app.exec()


def makeEnvelopes(data):
	data = np.abs(data)

	window = 100
	env = np.convolve(
		data,
		np.ones(window)/window,
		mode='same'
	)

	env2 = np.zeros_like(data)

	alpha = 0.005
	for i in range(1, len(data)):
		env2[i] = env2[i-1] + alpha * (abs(data[i]) - env2[i-1])

	displayData([data,env,env2])

def winfilt(data, window=100):
	return np.convolve(
		data,
		np.ones(window)/window,
		mode='same'
	)

def spectr(data, fs):
	N = len(data)
	spectrum = np.fft.rfft(data)
	frequencies = np.fft.rfftfreq(N, 1/fs)
	amplitude_spectrum = np.abs(spectrum) / N * 2

	app = QtWidgets.QApplication([])
	win = pg.GraphicsLayoutWidget(show=True)
	plot = win.addPlot()
	plot.showGrid(x=True, y=True)
	plot.plot(frequencies, amplitude_spectrum)
	app.exec()


def dsp(files):
	data = readFiles([files[0]])[0]
	dc = np.mean(data[int(len(data)/2):])
	data = np.clip(data - dc, -1000, 1000)

	fs = 500000

	# spectr(data, fs)
	# return

	
	fhet = 62000
	N = len(data)
	t = np.arange(N) / fs


	ref_sin = np.sin(2 * np.pi * fhet * t)
	ref_cos = np.cos(2 * np.pi * fhet * t)

	I = data * ref_cos
	Q = data * ref_sin

	b, a = butter(4, 2000 / (fs / 2), btype='low')
	I_f = filtfilt(b, a, I)
	Q_f = filtfilt(b, a, Q)

	I_f2 = winfilt(I,50)
	Q_f2 = winfilt(Q,50)

	complex1 = I_f + 1j * Q_f
	complex2 = I_f2 + 1j * Q_f2
	mag1 = np.abs(complex1)
	mag2 = np.abs(complex2)

	c_mag1 = np.clip(mag1, 0, 100)
	c_mag2 = np.clip(mag2, 0, 100)
	displayData([data, mag1, c_mag1, mag2, c_mag2])
	# spectr(magnitude, fs)
