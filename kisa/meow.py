import numpy as np
import sounddevice as sd
from libdsp import * 

def plot(ena,arr):
	if not ena:
		return
	createPlotWindow("Meow",arr)
	plotAll()

def eq(s1,s2):
	return s1[0:len(s2)] if len(s1) > len(s2) else s1



fs = 22050

tscale = 1

t0 = 0.1 * tscale
t1 = 0.3 * tscale
t2 = 0.1 * tscale
t3 = 0.1 * tscale

tsum = t0 + t1 + t2 + t3
tt = round(tsum * fs)

fmax = 500
f0 = fmax*0.5
f1 = fmax
f2 = fmax*0.9

g0 = gen_chirp(t0,f0,f1,fs)
g1 = gen_chirp(t1,f1,f1,fs)
g2 = gen_chirp(t2,f1,f2,fs)
g3 = gen_chirp(t3,f2,f2,fs)

sig = np.concatenate([g0,g1,g2,g3])
tone = gen_chirp(tsum+10,fmax,fmax,fs)
tone = eq(tone,sig)

# sig += tone

ssig = np.sign(sig)

amp = np.concatenate([
	np.logspace(0.0, 1.0, round(tt/2)),
	np.logspace(1, 0, int(tt/2)+10),
])/10

amp = eq(amp,ssig)


# ya = np.concatenate([
# 	np.zeros(round(tt/4)),
# 	np.logspace(0.0, 1.0, round(tt/4)),
# 	np.logspace(1, 0, round(tt/4)),
# 	np.zeros(round(tt/4)+10)
# ])
ya = np.concatenate([
	# np.zeros(round(tt/5)),	
	np.linspace(0.0, 1.0, round(tt*0.4)),
	np.full(round(tt*0.2),1),
	np.linspace(1.0, 0.0, round(tt*0.4)),
	# np.zeros(round(tt/5))
])

ya = eq(ya,sig)

# shapedsig = ya*ssig
# sig = amp*sig

# playsig = shapedsig/10
# playsig = (sig+shapedsig/10)/2
playsig = ssig*ya

plot(1,[
	amp,
	("amp",amp,1),
	("ya",ya,1),
	("sig",sig,1),
	("signed",ssig,1),
	# ("shaped",shapedsig,1),
	("played",playsig,1)
])

sd.play(np.concatenate([playsig, np.zeros(10000)]), samplerate=fs)
sd.wait()

