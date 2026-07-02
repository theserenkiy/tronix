import numpy as np
import sounddevice as sd
from libdsp import * 


fs = 44100

g1 = gen_chirp(0.3,1000,1000,fs)
g2 = gen_chirp(0.3,1000,700,fs)

sig = np.concatenate([g1,g2])
ssig = np.sign(sig)

amp = np.concatenate([
	np.linspace(0.0, 1.0, int(0.3*fs)),
	np.linspace(1, 0, int(0.3*fs)),
])

shapedsig = amp*ssig

createPlotWindow("Meow",[
	amp,
	("sig",sig,1),
	("signed",ssig,1),
	("shaped",shapedsig,1)
])
plotAll()



sd.play(sig, samplerate=fs)
sd.wait()
