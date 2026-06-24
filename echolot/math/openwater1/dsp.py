from libdsp import *
from lib import *
import sys
import os
import traceback
from scipy.signal import chirp

fs = 250000
f0 = 185000
fhet = 125000
pinglen_ms = 50
pinglen_samp = int(fs*pinglen_ms/1000)

f1 = f0-fhet

print("Ping len samples: ",pinglen_samp)

num = sys.argv[1] if len(sys.argv) > 1 else None

start_cycle = int(sys.argv[2] if len(sys.argv) > 2 else 0)

print(f"Start cycle = {start_cycle}")

end_cycle = 1
if start_cycle > 4:
	exit("Start cycle out of range")


def dsp_shift(sig,start_offset=3,end_offset=6):
	chunknums = []
	for i in range(start_cycle,end_cycle):
		chunknums += list(range(i*6+start_offset,i*6+end_offset))
	
	# print("Chunknums",chunknums)


	sig = getOnlyChunks(sig,pinglen_samp,chunknums)
	
	res = None
	for i in range(len(chunknums)):
		start = i*pinglen_samp
		zshift = quad_shift_abs(sig[start:start+pinglen_samp], 62000, fs)
		zshift = winfilt(zshift,400)
		if res is None:
			res = zshift
		else:
			res = res + zshift

	createPlotWindow(f"DSP SHIFT TO ZERO",[
		sig,
		np.abs(sig),
		# winfilt(asig,50)	# env
		# ref0
		res
	])

	# plotAll()

def dsp_corr(sig,start_offset=3,end_offset=6):
	chunknums = []
	for i in range(start_cycle,end_cycle):
		chunknums += list(range(i*6+start_offset,i*6+end_offset))
	
	sig = getOnlyChunks(sig,pinglen_samp,chunknums)

	# ref = genRefPack(10, 40, 4, fs, f0, f1) if typ==0 else 
	ref = genN(60,fs,f0,f1)
	
	res = None
	for i in range(len(chunknums)):
		start = i*pinglen_samp
		ping = sig[start:start+pinglen_samp]
		corr = np.correlate(ping, ref, mode="same")
		corr = np.abs(corr)	
		corr = winfilt(corr,400)
		if res is None:
			res = corr
		else:
			res = res + corr

	createPlotWindow(f"DSP CORR ",[
		sig,
		np.abs(sig),
		# winfilt(asig,50)	# env
		# ref0
		res
	])

	# plotAll()
	

def dsp_lfm(sig):

	sig = removeDC(sig)

	# Параметры
	chirp_dur = 1000e-6
	f0 = 160e3-fhet
	f1 = 200e3-fhet
	win = 10
	t = np.linspace(0, chirp_dur, int(chirp_dur * fs), endpoint=False) # Вектор времени от 0 до 2 сек

	# Генерация chirp-сигнала: от 100 Гц в начале до 1000 Гц через 2 секунды
	ref = chirp(t, f0=f0, t1=chirp_dur, f1=f1, method='linear', phi=270)
	ref12 = sig[104110:104350]
	ref12 /= np.linalg.norm(ref12)

	# ref12 = np.sign(ref)

	ref2 = genN(64,fs,f0,f1)

	

	res = None
	res2 = None
	for i in range(0,10):
		start = i*pinglen_samp
		ping = sig[start+6000:start+pinglen_samp]
		
		corr = correlate(ping,ref,win)
		corr2 = correlate(ping,ref12,win)
		
		if res is None:
			res = corr
			res2 = corr2
		else:
			res = res + corr
			res2 = res2 + corr2

	# for i in range(10,16):
	# 	start = i*pinglen_samp
	# 	ping = sig[start:start+pinglen_samp]
	# 	corr = np.correlate(ping, ref2, mode="same")
	# 	corr = np.abs(corr)	
	# 	corr = winfilt(corr,win)

	# 	if res2 is None:
	# 		res2 = corr
	# 	else:
	# 		res2 = res2 + corr

	# res3 = None
	# for i in range(6,12):
	# 	start = i*pinglen_samp
	# 	zshift = quad_shift_abs(sig[start:start+pinglen_samp], f1, fs)
	# 	zshift = winfilt(zshift,win)
	# 	if res3 is None:
	# 		res3 = zshift
	# 	else:
	# 		res3 = res3 + zshift


	createPlotWindow("Chirp",[
		sig,
		ref,
		ref12,
		np.abs(res),
		np.abs(res2),
		# np.abs(res2),
		# res3
	])


try:

	# sig = readWAVbyNum(num)
	f = open("test.wav","rb")
	f.seek(1088)
	sig = readSignalFromFp(f)
	f.close()
	# dsp(sig,0)
	# dsp(sig,1)
	# dsp_shift(sig,0,6)
	# dsp_corr(sig,0)
	# dsp_corr(sig,0,6)
	dsp_lfm(sig)

	plotAll()


except Exception as e:
	print(f"ERROR: {e}")
	traceback.print_exc()




