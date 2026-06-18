import sys
import time
import serial
import serial.tools.list_ports
import numpy as np
import matplotlib.pyplot as plt
import pyqtgraph as pg
from pyqtgraph.Qt import QtWidgets


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


def extractRawData(raw):
	full_view = memoryview(raw)

	start = raw.find(bytes(">>DATA", encoding="utf-8"))
	stop  = raw.find(bytes(">>DATAEND", encoding="utf-8"))

	if start < 0:
		# print("Начало данных куда-то проебали. Это очень хуёво...")
		print("Начало данных куда-то проебали. Берем весь кусок!")
		start = 0
		# exit()

	if stop < 0:
		print("Конец данных куда-то проебали. Да и хуй с ним!")
		stop = len(raw)

	if (stop-start)%2:
		stop -= 1
	
	return full_view[start+6:stop]



def prepareRawData(raw):
	data = extractRawData(raw)

	return np.frombuffer(data, dtype=np.uint16)


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


