import numpy as np
import matplotlib.pyplot as plt
import pyqtgraph as pg
from pyqtgraph.Qt import QtWidgets
import os
import signal

signal.signal(signal.SIGINT, signal.SIG_DFL)

def readWAVbyNum(num):
	if not num:
		raise Exception("No file number given")

	fname = f'save/save_{num.rjust(6,"0")}.wav'

	print(f"Trying to open {fname}")

	if not os.path.exists(fname):
		raise Exception("File not found (")

	with open(fname,"rb") as f:
		f.seek(44)
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

	win = pg.GraphicsLayoutWidget(show=True)
	win.setWindowTitle(title)

	plot1 = None
	for data in datas:
		plot = win.addPlot()
		if xsync:
			if not plot1:
				plot1 = plot
			else:
				plot.setXLink(plot1)

		plot.showGrid(x=True, y=True)
		plot.setMouseEnabled(x=True, y=False)
		curve = plot.plot(data)
		win.nextRow()
	win.show()
	wins.append(win)

	# app.exec()

def plotAll():
	global app
	if not app:
		return
	app.exec()

