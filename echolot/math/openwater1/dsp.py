from libdsp import *
from lib import *
import sys
import os
import traceback

fs = 250000
f0 = 187000
fhet = 125000
pinglen_ms = 40
pinglen_samp = int(fs*pinglen_ms/1000)

f1 = f0-fhet

num = sys.argv[1] if len(sys.argv) > 1 else None

measnum = int(sys.argv[2] if len(sys.argv) > 2 else 0)

print(f"Using measnum = {measnum}")

if measnum > 4:
	exit("Measnum out of range")


def dsp_shift(sig):
	chunknums = []
	for i in range(5):
		chunknums += list(range(i*6+3,i*6+6))
	
	sig = getOnlyChunks(sig,pinglen_samp,chunknums)
	
	res = None
	for i in range(15):
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

	plotAll()
	


def dsp(sig,typ):
	start = measnum*6
	chunknums = list(range(start,start+3) if typ==0 else range(start+3,start+6))

	sig = getOnlyChunks(sig,pinglen_samp,chunknums)
	sig = removeDC(sig)
	asig = np.abs(sig)


	ref = genRefPack(10, 40, 4, fs, f0, f1) if type==0 else genN(60,fs,f0,f1)

	corr = np.correlate(sig, ref, mode='full')
	corr = np.abs(corr)	

	zshift = quad_shift_abs(sig, 62000, fs)

	createPlotWindow(f"DSP type {typ}",[
		sig,
		asig,
		# winfilt(asig,50)	# env
		# ref0
		corr,
		winfilt(corr,400),
		zshift,
		winfilt(zshift,400),
	])



try:

	sig = readWAVbyNum(num)
	# dsp(sig,0)
	# dsp(sig,1)
	dsp_shift(sig)
	plotAll()


except Exception as e:
	print(f"ERROR: {e}")
	traceback.print_exc()



