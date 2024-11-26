from tkinter import *
import math

def genSin(freq:int,amp:float,leng:int=100):
	sig = []
	amp = amp if amp >= 0 and amp <= 1 else 1
	for i in range(leng):
		sig.append(math.sin(i*freq/100)*amp)
	return sig

def plotSig(sig,canvW,canvH):
	chm = int(canvH/2)
	last = [0,sig[0]]
	for idx, v in enumerate(sig):
		if idx == 0:
			continue
		cn.create_line(last[0],chm-last[1]*chm,idx,chm-v*chm)
		last = [idx,v]

def sumSig(siglist):
	sig = siglist[0].copy()
	for signum in range(len(siglist)-1):
		for i,v in enumerate(siglist[signum+1]):
			sig[i] += v
	return sig

def fft(sig,ran=[5,7]):
	out = []
	for win in range(ran[1],ran[0],-1):
		print(win)
		wts = sig[0:win]
		for j,v in enumerate(sig[win:]):
			wts[j%win] += v
			
		print(wts)
		out.append(sum(wts))
	return out

ch = 300
cw = 500
chm = int(ch/2)

root = Tk()
root.title("FFT")
root.geometry(str(cw)+"x"+str(ch))


cn = Canvas(bg="white", width=cw, height=ch)
cn.pack(anchor=CENTER, expand=1)
cn.create_line(0,chm,cw,chm)

sig1 = genSin(10,0.6,50)
sig2 = genSin(23,0.2,50)
# sig3 = genSin(47,0.1,cw)
sig = sumSig([sig1,sig2])
print(sig)
# plotSig(sig1,cw,ch)
# plotSig(sig2,cw,ch)

plotSig(sig,cw,ch)
sig = fft(sig)
print(sig)

root.mainloop()