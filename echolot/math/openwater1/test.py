from libdsp import *
from lib import *
import sys
import os
import traceback

sig = None
with open("chirp.bin","rb") as f:
	sig = readSignalFromFp(f)

createPlotWindow("Chirp",[sig])
plotAll()