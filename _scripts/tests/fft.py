from tkinter import *
import math
import sys

ch = 300
cw = 1000
cwm = int(cw/2)
chm = int(ch/2)
fs = 1000

ts = 1/fs

def genSin(freq:int,amp:float=1,leng:int=1000,phase:float=0):
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

def sumSig(siglist):
	sig = siglist[0].copy()
	for signum in range(len(siglist)-1):
		for i,v in enumerate(siglist[signum+1]):
			sig[i] += v
	return sig

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

def getWinWeights(sig,win):
	hwin = int(win/2)
	wts = [0]*win
	points = []
	absmax = 0
	angstep = 2*math.pi/win
	for j,v in enumerate(sig):
		ind0 = j%win
		ind1 = (ind0+hwin)%win
		ind = ind0 if v >= 0 else ind1
		a = abs(v)
		points.append(a)#[int(math.cos(ind*angstep)*a*100),-int(math.sin(ind*angstep)*a*100)])
		if wts[ind] < a:
			wts[ind] = a

		if a > absmax:
			absmax = a

		# p0 = wts[ind0] + v
		# p1 = wts[ind1] - v
		# a0 = abs(wts[ind0])
		# a1 = abs(wts[ind1])
		
		# if a1 > absmax:
		# 	absmax = a1
		# wts[ind0] = p0
		# wts[ind1] = p1

	# print(points)
		
	# print(wts)
	wts = [x/absmax for i,x in enumerate(wts)]
	meds = []
	for i,x in enumerate(wts):
		meds.append(x - wts[(i+hwin)%win])

	# last = None
	# for pt in points:
	# 	if last:
	# 		cn.create_line(cwm+last[0], chm+last[1], cwm+pt[0], chm+pt[1], fill="blue")
	# 	last = pt

	# plotRound(points,"blue",angstep)
	plotRound(wts,"red")
	plotRound(meds,"green")

	
	avgw = sum(wts)/win
	print(f'Win = {win}; Avg wt = {avgw}')
	comp = []
	for i in range(hwin):
		comp.append((wts[i]+wts[i+hwin])/2)
	# plotSig(comp,cw,ch,color="red")
	# plotSig(wts,cw,ch,color="green")

	return avgw
	


root = Tk()
root.title("FFT")
root.geometry(str(cw)+"x"+str(ch))

cn = Canvas(bg="white", width=cw, height=ch)
cn.pack(anchor=CENTER, expand=1)
cn.create_line(0,chm,cw,chm,fill="green")
cn.create_line(cwm,0,cwm,ch,fill="green")

sig1 = genSin(10,0.4,1000,float(sys.argv[1]))
# sig2 = genSin(10,0.2,1000)
# sig2 = genSin(20,0.1,1000)
# sig3 = genSin(40,0.5,1000)
# sig = sumSig([sig1,sig2,sig3])

plotSig(fft(sig))

root.mainloop()