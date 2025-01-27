from tkinter import *
import math
import sys

ch = 300
cw = 2000
cwm = int(cw/2)
chm = int(ch/2)
fs = 1000

ts = 1/fs

colors = "red", "chocolate3", "green", "blue", "blue violet"

def genSin(freq:int,amp:float=1,leng:int=2000,phase:float=0):
	sig = []
	phase *= math.pi
	amp = amp if amp >= 0 and amp <= 1 else 1
	for n in range(leng):
		sig.append(amp * math.sin(phase+2*math.pi*freq*ts*n))
	return sig

def plotSig(sig,scale=1,color="black"):
	chm = int(ch/2)
	last = [0,sig[0]]
	for idx, v in enumerate(sig):
		if idx == 0:
			continue
		cn.create_line(last[0],chm-last[1]*chm,idx,chm-v*scale*chm, fill=color)
		last = [idx,v]

def plotRound(sig,color="red",angstep=0):
	if not angstep:
		angstep = 2*math.pi/len(sig)

	last = None
	for i,v in enumerate(sig):
		if v < 0:
			continue
		pt = [int(math.cos(i*angstep)*v*100),-int(math.sin(i*angstep)*v*100)]
		if last:
			cn.create_line(cwm+last[0], chm+last[1], cwm+pt[0], chm+pt[1], fill=color)
		last = pt

def sumSig(siglist,minus=0):
	sig = siglist[0].copy()
	for signum in range(len(siglist)-1):
		for i,v in enumerate(siglist[signum+1]):
			sig[i] += v
	return sig

def subSig(sig1,sig2):
	out = []
	s2len = len(sig2)
	for i,v in enumerate(sig1):
		if i >= s2len:
			break
		out.append(v-sig2[i])
	return out

def multSig(sig1,sig2):
	sig = []
	for i,v in enumerate(sig1):
		sig.append(v*sig2[i])
	return sig

def avgSig(sig):
	return sum(sig)/len(sig)

def fft(sig,ran=[5,50]):
	out = []
	for freq in range(ran[0],ran[1]):
		sigt1 = genSin(freq)
		sigt2 = genSin(freq,phase=0.5)
		sigm1 = multSig(sig,sigt1)
		sigm2 = multSig(sig,sigt2)
		res = []
		for i,v in enumerate(sigm1):
			res.append(math.sqrt(v**2+sigm2[i]**2))
		out.append(avgSig(res))
	return out

def lowpass(sig,freq):
	winsize = round(fs/freq/2)
	hwin = winsize/2
	print("winsize",winsize)
	win = [0] * winsize
	out = []
	i = 0
	for v in sig:
		win[i%winsize] = v
		if i >= hwin:
			out.append(sum(win)*1.4/winsize)
		i += 1
	return out

def analyze(sig,freqs):
	subsig = sig[:]
	plotSig(sig,color="gray")
	filtered = None
	out = {}
	i = 0
	for f in freqs:
		filtered = lowpass(subsig,f)
		plotSig(filtered,color=colors[i%5])
		out[f] = sum([abs(x) for x in filtered])/len(filtered)
		subsig = subSig(subsig,filtered)
		# plotSig(subsig,color=colors[i%5])
		i+=1
		
	return out

root = Tk()
root.title("Test")
root.geometry(str(cw)+"x"+str(ch))

cn = Canvas(bg="white", width=cw, height=ch)
cn.pack(anchor=CENTER, expand=1)
cn.create_line(0,chm,cw,chm,fill="green")
cn.create_line(cwm,0,cwm,ch,fill="green")

#sig = genSin(10,0.4,1000,float(sys.argv[1]))
sig1 = genSin(10,0.3)
sig2 = genSin(44,0.5)
sig3 = genSin(60,0.3)

# sig3 = genSin(40,0.5,1000)
sig = sumSig([sig1,sig2,sig3])

an = analyze(sig,[10,20,30,40,50,60,70])
print(an)

root.mainloop()