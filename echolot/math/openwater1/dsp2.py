from libdsp import *
from lib import *
import sys
import os
import traceback
from scipy.signal import chirp

onlyTests = parseArgToList(2,-1)


data_offset = 0x840


info = None
sig = None
fnum = sys.argv[1] if len(sys.argv) > 1 else 0

if not fnum:
	exit("File num not specified")
fname = getWAVNameByNum(fnum)
print("FNAME ",fname)
preset = fname[17:-4]

f =  open(fname,"rb")
info = readWAVInfo(f)

testnums = list(range(len(info["tests"]))) if not onlyTests else onlyTests

print("TESTNUMS: ",testnums)

sigs = []
corrs = []
quad_sum = None
env_sum = None
for testnum in testnums:
	test = info["tests"][testnum]
	sigs = []
	fs = info["fs"]
	pinglen_samp = int(info["ping_samples"])
	pinglen_bytes = pinglen_samp * 2
	fhet = 250000

	typ = test["type"]
	dur = test["dur"]
	f0 = abs(test["f0"]-fhet)
	f1 = abs(test["f1"]-fhet) if test["f1"] else 0

	if typ=="F":
		typ = "L"
		f1 = f0
	if typ=="L":
		name = f"CHIRP: {int(dur*1e6)}us {int(test['f0']/1000)}-{int(test['f1']/1000)}"
	elif typ=="P":
		name = f"PSK: {int(test['symdur']*1e6)}us/sym {f0} {test['pattern']}"

	print(f"TEST #{testnum}: {name}")	
	if not onlyTests:
		continue
		
	
	pran = test["pingrange"]
	f.seek(data_offset + pinglen_bytes*pran[0], 0)
	
	
	corr_sum = None
	sigs = []
	for i in range(test["npings"]):
		sig = prepareRawSig(f.read(pinglen_bytes))
		sigs.append(sig)

		quad = quad_shift_abs(sig,f0,fs)
		quad_sum = sigsum(quad_sum,quad)

		env = get_env(sig)
		env_sum = sigsum(env_sum,env)

		ref = None

		if typ=="L":			
			ref = gen_chirp(dur, f0, f1, fs)
		elif typ=="P":
			ref = gen_psk(f0, fs, test["symdur"], test["pattern"])
		else:
			exit(f"Unknown type {typ}")

		corr = correlate(sig,ref)
		corr_sum = corr_sum + corr if corr_sum is not None else corr
	
	corrs.append((f"#{testnum} {name}", corr_sum))

	

createPlotWindow(f"{fnum} ({preset}) {info['depth']}m",
[
	np.concatenate(sigs),
	("Env",env_sum),
	("quad",quad_sum),
	
	# winfilt(quad_sum),
	# ref,
]
+ corrs
)
plotAll()