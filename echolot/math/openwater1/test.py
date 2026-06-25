from libdsp import *
from lib import *
import sys
import os
import traceback

fs = 10000;

sig = None
with open("chirp.bin","rb") as f:
	sig = readSignalFromFp(f)-0.5

ref = genT(len(sig)/fs,10000,200)

createPlotWindow("Chirp",[sig, ref, sig*ref])
plotAll()