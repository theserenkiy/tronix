import sys
import serial
import serial.tools.list_ports
import numpy as np
import matplotlib.pyplot as plt
import pyqtgraph as pg
from pyqtgraph.Qt import QtWidgets



def serialGet(bytes, file=None):
	ports = [port for port, desc, hwid in sorted(serial.tools.list_ports.comports(), reverse=True)]
	print(f"Available ports: {ports}")

	if not len(ports):
		print("No ports found")
		exit()

	port = ports[0]
	if port == "COM1":
		print("No port found except COM1")
		exit()

	print(f"Connecting to {port}....")

	ser = serial.Serial(port, 115200)

	print("Connected! Reading")

	raw = ser.read(bytes)
	if file:
		with open(file,"wb") as f:
			f.write(raw)

	return raw




def displayRawData(raw=None, file=None):
	if file:
		with open(file,"rb") as f:
			raw = f.read()

	if not raw:
		exit("Missing raw data")

	full_view = memoryview(raw)

	start = raw.find(bytes(">>DATA", encoding="utf-8"))
	stop  = raw.find(bytes(">>DATAEND", encoding="utf-8"))

	if start < 0:
		print("Начало данных куда-то проебали...")
		exit()

	if stop < 0:
		print("Конец данных куда-то проебали. Да и хуй с ним!")
		stop = len(raw)

	data = np.frombuffer(full_view[start+6:stop], dtype=np.uint16)

	# start = data.

	# plt.figure(figsize=(14, 8))
	# plt.plot(data)

	# plt.grid(True)
	# plt.tight_layout()

	# plt.show()

	app = QtWidgets.QApplication([])

	win = pg.GraphicsLayoutWidget(show=True)
	plot = win.addPlot()

	plot.showGrid(x=True, y=True)

	curve = plot.plot(data)

	app.exec()


