from libdsp import *
from lib import *
import sys
import os
import traceback
from scipy.signal import chirp


with open(getWAVNameByArg(),"rb") as f:
	readWAVInfo(f)


fs = 500000
f0 = 214000
fhet = 250000
pinglen_ms = 50
pinglen_samp = int(fs*pinglen_ms/1000)

f1 = abs(f0-fhet)