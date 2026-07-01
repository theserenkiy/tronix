import pyqtgraph as pg
from pyqtgraph.Qt import QtWidgets
from scipy.signal import butter, filtfilt
import numpy as np
from lib import *
import os
from scipy.signal import chirp

def sigsum(sig1,sig2):
	return sig2 if sig1 is None else sig1+sig2

def prepareRawSig(sig):
	# print("PREPARE RAW ",len(sig))
	return removeDC(np.frombuffer(sig, dtype=np.int16))

def correlate(sig,ref,win=100):
	corr = np.correlate(sig, ref, mode="same")
	corr = np.abs(corr)	
	return winfilt(corr,win)

def removeDC(sig):
	dc = np.mean(sig[int(len(sig)/2):])
	return sig - dc


def winfilt(data, window=100):
	return np.convolve(
		data,
		np.ones(window)/window,
		mode='same'
	)

def winfilt2(data, alpha=0.005, imba=1):
	env2 = np.zeros_like(data)
	alpha2 = alpha*imba

	for i in range(1, len(data)):
		delta = abs(data[i]) - env2[i-1]
		env2[i] = env2[i-1] + (alpha if delta < 0 else alpha2) * delta
	
	return env2


def get_env(sig, win=100):
	asig = np.abs(sig)
	return winfilt(asig, win)


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

def genN(N,fs,f0,f1):
	Ns = int(round(N * fs / f0))
	t = np.arange(Ns) / fs
	return np.sin(2*np.pi*f1*t)

def genT(T,fs,f):
	Ns = int(T*fs)
	t = np.arange(Ns) / fs
	return np.sin(2*np.pi*f*t)

def genRefPack(N_puls, delay_us, N_cycles, fs, f0, f1):
	ref = genN(10,fs,f0,f1)
	zz = np.zeros(int(delay_us/(1e6/fs)))
	conlist = [ref, zz]*(N_cycles-1)
	conlist.append(ref)
	return np.concatenate(conlist)

def gen_chirp(chirp_dur,f0,f1,fs):
	win = 10
	t = np.linspace(0, chirp_dur, int(chirp_dur * fs), endpoint=False) # Вектор времени от 0 до 2 сек

	# Генерация chirp-сигнала: от 100 Гц в начале до 1000 Гц через 2 секунды
	return chirp(t, f0=f0, t1=chirp_dur, f1=f1, method='linear', phi=270)


def gen_psk(f,fs,symlen,code):
	# 1. Параметры сигнала
	sps = symlen*fs             # Количество отсчетов (samples) на один бит
	fc = f*symlen               # Несущая частота (количество периодов на бит)

	# 2. Генерация случайных битов (0 или 1)
	bits = np.array(code)

	# 3. Преобразование в BPSK символы: 0 -> 1, 1 -> -1
	symbols = 1 - 2 * bits

	# 4. Повторение каждого символа (формирование прямоугольных импульсов)
	baseband = np.repeat(symbols, sps)

	# 5. Генерация несущей частоты
	t = np.arange(len(baseband)) / sps
	carrier = np.cos(2 * np.pi * fc * t)

	# 6. BPSK сигнал (умножение базовой полосы на несущую)
	return baseband * carrier

def simple_shift(sig, fhet, fs, win=0.001):
	N = len(sig)
	t = np.arange(N) / fs
	ref = np.sign(np.sin(2 * np.pi * fhet * t))
	prod = ref*sig
	# return winfilt2(ref * sig, win)
	return prod
	

def quad_shift(sig, fhet, fs):
	N = len(sig)
	t = np.arange(N) / fs
	
	ref_sin = np.sin(2 * np.pi * fhet * t)
	ref_cos = np.cos(2 * np.pi * fhet * t)
	
	I = sig * ref_cos
	Q = sig * ref_sin

	b, a = butter(4, 10000, fs=fs, btype='low')
	# I_f = filtfilt(b, a, I)
	# Q_f = filtfilt(b, a, Q)

	I_f = winfilt(I, 200)
	Q_f = winfilt(Q, 200)
	
	
	return I_f + 1j * Q_f
	return I + 1j * Q

def quad_shift_abs(sig, fhet, fs):
	return np.abs(quad_shift(sig, fhet, fs))


def dsp_old(files):
	data = readFiles([files[0]])[0]
	# data = data[]
	dc = np.mean(data[int(len(data)/2):])

	data = data - dc

	adata = np.abs(data)

	env2 = winfilt2(data,0.01)

	fs = 250000

	# spectr(data, fs)
	# return

	
	fhet = 62000
	


	

	I_f2 = winfilt(I,100)
	Q_f2 = winfilt(Q,100)

	
	complex2 = I_f2 + 1j * Q_f2
	mag1 = np.abs(complex1)
	mag2 = np.abs(complex2)

	c_mag1 = mag1 #np.clip(mag1, 0, 100)
	c_mag2 = mag2 #np.clip(mag2, 0, 100)

	ref0 = genN(32,fs,fhet)
	ref = genN(10,fs,fhet)
	zz = np.zeros(int(40/(1e6/fs)))
	ref2 = np.concatenate((ref, zz, ref, zz, ref, zz, ref))


	corr = np.correlate(data, ref0[::-1], mode='same')
	corr = np.abs(corr)
	# corr = np.clip(corr,0,2000)

	mcorr = mancorr(data,ref)
	mcenv = winfilt(mcorr,4000)
	fmcorr = winfilt(mcorr,20)
	
	fmag = winfilt2(c_mag2,0.005)
	mmag = winfilt2(fmag,0.0005)




	# displayData(
	# 	c_mag2[0:10000] + c_mag2[10000:20000]
	# )

	displayData([
		# refsig
		data, 
		adata, 
		# env2, 
		# mag2, 
		# c_mag2, #[0:10000] + c_mag2[10000:20000]+ c_mag2[20000:30000]+ c_mag2[30000:40000], 
		# fmag,
		# mmag,
		# fmag-mmag
		# norm(c_mag2,0.00001,0.005)
		
		corr,
		# mcorr,
		# mcenv,
		# fmcorr,
		# fmcorr-mcenv
	])
	# spectr(magnitude, fs)
