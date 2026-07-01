from lib import *
from libdsp import *

# ffmpeg -i audio_2026-07-01_01-58-19.ogg -c:a pcm_s16le -ac 1 -ar 22050 test1.wav

fs = 44100

def my_hp(sig,win):
	lp = winfilt(sig,win)
	return sig-lp

def main():
	sig = None
	with open("test2.wav","rb") as f:
		f.seek(76,0)
		sig = prepareRawSig(f.read())

	kiss = []
	fhet = 5000
	# for fhet in range(2000,8000,1000):
	# sh = quad_shift_abs(sig,fhet,fs)
	# het = genSameLen(sig,fhet,fs)
	# shet = np.sign(het)
	# prod = sig*shet
	# pfilt = winfilt(prod,500)
	# env = winfilt(prod,4000)
	# hpass = pfilt-env

	
	bpass = []
	f0 = 5000
	# for f0 in range(2000,10000,1000):
	for q in range(1,10):
		s = mfb_bpass(sig,f0,q,fs)
		s = winfilt(np.abs(s),2000)
		env = winfilt(s,10000)
		res = np.sign(s-env+20)
		# bpass.append(("Q "+str(q),s))
		bpass.append(("Q "+str(q),res))
	

	createPlotWindow("Q",[
		sig,

		# ("shet",shet,1),
		# env,
		# ("prod",prod,1),
		# ("pfilt",pfilt,1),
		# ("env",env,1),
		# ("hpass",np.abs(hpass),1)
		# sh2,
		# np.sign(sh2)
		
	]
	 +bpass
	)

	plotAll()


def filter_test():
	sig = gen_chirp(4,1000,10000,fs)
	hp = mfb_hpass(sig,5000,5,fs)
	bp = mfb_bpass(sig,5000,5,fs)
	createPlotWindow("FILTERED",[
		sig,
		hp,
		bp
	])
	plotAll()



# filter_test()
main()