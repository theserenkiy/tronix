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

def winfilt2(data, alpha=0.005, imba=10):
	env2 = np.zeros_like(data)
	alpha2 = alpha*imba

	for i in range(1, len(data)):
		delta = abs(data[i]) - env2[i-1]
		env2[i] = env2[i-1] + (alpha if delta < 0 else alpha2) * delta
	
	return env2


def mancorr(sig, pattern):
	pattern = pattern[::2]
	sig = sig[::2]
	res = np.zeros_like(sig, dtype=np.float64)
	# win = np.zeros_like(pattern, dtype=np.float64)

	plen = len(pattern)
	for start in range(len(sig)-plen):
		res[start] = np.mean(sig[start:start+plen] * pattern)

	return np.abs(res)


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


def norm(data, meanalpha, filtalpha):
	flt = winfilt2(data,filtalpha)
	mean = winfilt2(flt,meanalpha)
	return flt-mean


def dsp(files):
	data = readFiles([files[0]])[0]
	data = data[]
	dc = np.mean(data[int(len(data)/2):])

	data = data - dc

	adata = np.abs(data)

	env2 = winfilt2(data,0.01)

	fs = 250000

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

	Ncycles = 32

	Ns = int(round(Ncycles * fs / fhet))

	t = np.arange(Ns) / fs

	ref = np.sin(2*np.pi*fhet*t)

	corr = np.correlate(data, ref[::-1], mode='same')
	corr = np.abs(corr)
	# corr = np.clip(corr,0,2000)

	mcorr = mancorr(data,ref)
	mcenv = winfilt(mcorr,4000)
	fmcorr = winfilt(mcorr,20)
	
	fmag = winfilt2(c_mag2,0.005)
	mmag = winfilt2(fmag,0.0005)

	displayData([
		data, 
		adata, 
		env2, 
		# mag2, 
		c_mag2, 
		fmag,
		mmag,
		fmag-mmag
		# norm(c_mag2,0.00001,0.005)
		
		# corr,
		# mcorr,
		# mcenv,
		# fmcorr,
		# fmcorr-mcenv
	])
	# spectr(magnitude, fs)
