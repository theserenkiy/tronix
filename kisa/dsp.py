from lib import *
from libdsp import *

# ffmpeg -i audio_2026-07-01_01-58-19.ogg -c:a pcm_s16le -ac 1 -ar 22050 test1.wav

fs = 22050


sig = None
with open("test1.wav","rb") as f:
	f.seek(76,0)
	sig = prepareRawSig(f.read())


kiss = []
fhet = 5000
# for fhet in range(2000,8000,1000):
# sh = quad_shift_abs(sig,fhet,fs)
sh = simple_shift(sig,5000,fs,0.0001)
# sh = winfilt2(sh,0.001)
env = winfilt2(sh,0.0001,2)
sh2 = sh-env+10
# kiss.append((str(fhet),sh))


createPlotWindow("Q",[
	sig,
	# env,
	("shifted",sh),
	# sh2,
	# np.sign(sh2)
	
])

plotAll()

