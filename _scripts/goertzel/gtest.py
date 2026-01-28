import numpy as np
import matplotlib.pyplot as plt
import os

os.system('title GTEST')

sr = 44100
# sr = 15

def getTimeLine(dur):
	return np.linspace(0,dur,int(sr*dur),endpoint=False)

def genSine(time,freq,ampl,phase=0):
	sig = ampl * np.sin(phase + 2 * np.pi * freq * time)
	return sig

def goertzel(data,freq):
	N = len(data)
	k = int(0.5 + (N * freq) / sr)
	w = 2 * np.pi * k / N
	cosine = np.cos(w)
	coef = 2 * cosine

	s_prev = 0.0
	s_prev2 = 0.0

	for n in range(N):
		s_cur = data[n] + coef * s_prev - s_prev2
		s_prev2 = s_prev
		s_prev = s_cur
	
	power = np.sqrt(s_prev2**2 + s_prev**2 - coef * s_prev *s_prev2)
	return power

time = getTimeLine(1)
sig = sum([
	genSine(time,60,100),
	genSine(time,1000,30),
	genSine(time,500,40),
	genSine(time,2000,100),
	genSine(time,1500,50),
	# genSine(time,21,0.8),
	# genSine(time,27,0.5),
	# genSine(time,33,0.3),
	# genSine(time,39,0.2),
	# genSine(time,45,0.13)
])

slice_size = 1024
bandwidth = sr/slice_size
subsig = list(sig[0:slice_size])
# print(subsig)
freqs = range(50,2000,int(bandwidth/1.2))
print("Points: ",len(freqs))
res = []
max = 0
for freq in freqs:
	p = goertzel(subsig,freq)
	res.append([freq,p])
	if p > max:
		max = p

for v in res:
	print(v[0],"\t", "="*round(20*v[1]/max))

# plt.figure(figsize=(10, 4))
# plt.plot(time, sig)
# plt.title('Generated Sine Wave')
# plt.xlabel('Time (s)')
# plt.ylabel('Amplitude')
# plt.grid(True)
# plt.show()