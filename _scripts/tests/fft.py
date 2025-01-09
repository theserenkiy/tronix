from tkinter import *
import math
import sys

ch = 300
cw = 1000
cwm = int(cw/2)
chm = int(ch/2)

def genSin(period:int,amp:float,leng:int=100):
	sig = []
	amp = amp if amp >= 0 and amp <= 1 else 1
	for i in range(leng):
		sig.append(math.sin(i*2*math.pi/period)*amp)
	return sig

def plotSig(sig,canvW,canvH,color="black"):
	chm = int(canvH/2)
	last = [0,sig[0]]
	for idx, v in enumerate(sig):
		if idx == 0:
			continue
		cn.create_line(last[0],chm-last[1]*chm,idx,chm-v*chm, fill=color)
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

def fft(sig,ran=[5,7]):
	out = []
	for win in range(ran[0],ran[1]):
		# print(win)
		out.append(getWinWeights(sig,win))
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
cn.create_line(0,chm,cw,chm)
cn.create_line(cwm,0,cwm,ch)

sig1 = genSin(500,0.6,10000)
sig2 = genSin(777,0.2,10000)
sig3 = genSin(1150,0.4,10000)
sig = sumSig([sig1,sig2,sig3])
# print(sig)
# plotSig(sig,cw,ch)
# plotSig(sig2,cw,ch)

wts = getWinWeights(sig,int(sys.argv[1]))



# plotSig(wts,cw,ch)

#sig = fft(sig,[5,int(sys.argv[1])])
# print(sig)

# plotSig(sig,cw,ch,"red")

root.mainloop()