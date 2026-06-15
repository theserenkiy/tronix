import pyqtgraph as pg
from pyqtgraph.Qt import QtWidgets
from scipy.signal import butter, filtfilt
import numpy as np
from lib import *

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

	data = data - dc

	adata = np.abs(data)

	env2 = np.zeros_like(adata)
	alpha = 0.005
	for i in range(1, len(adata)):
		env2[i] = env2[i-1] + alpha * (abs(adata[i]) - env2[i-1])

	

	


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

	I_f2 = winfilt(I,100)
	Q_f2 = winfilt(Q,100)

	complex1 = I_f + 1j * Q_f
	complex2 = I_f2 + 1j * Q_f2
	mag1 = np.abs(complex1)
	mag2 = np.abs(complex2)

	c_mag1 = np.clip(mag1, 0, 100)
	c_mag2 = np.clip(mag2, 0, 100)
	displayData([data, adata, env2, mag2, c_mag2])
	# spectr(magnitude, fs)
