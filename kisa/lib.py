import numpy as np
import matplotlib.pyplot as plt
import pyqtgraph as pg
from pyqtgraph.Qt import QtWidgets
import os
import signal
import sys
import re

signal.signal(signal.SIGINT, signal.SIG_DFL)

from pathlib import Path

def parseArgToList(n,dflt):
	if len(sys.argv) <= n:
		return None
	v = sys.argv[n]
	mm = re.findall(r"(\d+)\-(\d+)",v)
	if len(mm):
		return list(range(int(mm[0][0]), int(mm[0][1])+1))
	lst = v.split(",")
	return [int(x) for x in lst]
	

def getWAVNameByArg(argnum=1):
	num = sys.argv[argnum] if len(sys.argv) > argnum else 0
	if not num:
		exit("File num not specified")
	return getWAVNameByNum(num)


def getWAVNameByNum(num):
	directory = Path('./save')

	# 1. Non-recursive (only in the current directory)
	files = directory.glob(f'save_{str(num).rjust(6,"0")}*.wav')

	# Convert the generator to a list of strings if needed
	file_paths = [str(file) for file in files]

	if not len(file_paths):
		exit("File not found")
	
	return file_paths[0]


def readWAVInfo(f):
	f.seek(0x38, 0)
	s = f.read(2048).split(b'\x00')[0].decode('utf-8')

	d = {}
	for k in ["depth","ping_samples","fs"]:
		mm = re.findall(rf"{k} ([\d\.]+)",s)
		d[k] = float(mm[0]) if len(mm) else -1

	mm = re.findall(r"^([LFSP]) (\d+) (\d+) (\d+) (\d+) (\d+) (\d+) (\d+) ([a-f\d]+)",s,re.MULTILINE)

	mn=0
	pingn = 0
	d["tests"] = []
	d["pings"] = []


	for m in mm:
		pattern = [int(x) for x in list(f"{int(m[8],16):b}"[0:int(m[7])])]
		d["tests"].append({
			"type": m[0],
			"npings": int(m[1]),
			"pingrange": [pingn,pingn+int(m[1])],
			"f0": (int(m[2])*1000),
			"f1": (int(m[3])*1000),
			"dur": (int(m[4])*1e-6),
			"symdur": (int(m[5])*1e-6),
			"pattern": pattern
		})
		for i in range(int(m[1])):
			d["pings"].append(mn)
			pingn+=1
		mn+=1;


	# print(d)
	return d

def readWAVbyNum(num):
	if not num:
		raise Exception("No file number given")

	# fname = f'save/save_{num.rjust(6,"0")}.wav'

	# Define the target directory
	fname = getWAVNameByNum(num)

	print(f"Trying to open {fname}")

	if not os.path.exists(fname):
		raise Exception("File not found (")

	with open(fname,"rb") as f:
		f.seek(2110)
		return readSignalFromFp(f)
	
def readSignalFromFp(f):
	return np.frombuffer(f.read(), dtype=np.uint16)

def getOnlyChunks(sig,chunkSize,nums):
	conclist = []
	for num in nums:
		start = chunkSize*num
		conclist.append(sig[start:start+chunkSize])
	return np.concatenate(conclist)

app = None
wins = []
def createPlotWindow(title,datas):
	global app
	xsync = 0
	# start = data.

	# plt.figure(figsize=(14, 8))
	# plt.plot(data)

	# plt.grid(True)
	# plt.tight_layout()

	# plt.show()

	if not app:
		app = QtWidgets.QApplication([])

	win = pg.GraphicsLayoutWidget(show=True, size=(1200, 800))
	win.setWindowTitle(title)

	plot1 = None
	first = True
	for data in datas:
		title = None
		if type(data) == tuple:
			(title, data) = data
		plot = win.addPlot(title=title)
		if xsync:
			if not plot1:
				plot1 = plot
			else:
				plot.setXLink(plot1)

		plot.showGrid(x=True, y=True)
		plot.setMouseEnabled(x=first, y=not first)
		curve = plot.plot(data)
		win.nextRow()
		first = False
	win.show()
	wins.append(win)

	# app.exec()

def plotAll():
	global app
	if not app:
		return
	app.exec()

