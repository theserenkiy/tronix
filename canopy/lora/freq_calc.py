import sys
import math

arglen = len(sys.argv)
if arglen < 4:
	#print("Enter frequencies in MHz divided by space (433.321 434.567 446.112 etc)")
	print("Enter first frequency in MHz, step and count of steps divided by space (433.321 0.025 8)")
	sys.exit()

pll_step = 32e6/pow(2,19)
startfreq = float(sys.argv[1])
step = float(sys.argv[2])
index = 0
if arglen > 4:
	index = int(sys.argv[4])
for i in range(0,int(sys.argv[3])):
	freq = startfreq+(i*step)
	val = math.floor(freq*1e6/pll_step)
	hexval = hex(val)
	h  = str(hexval)
	print(".db 0x"+h[2:4]+", 0x"+h[4:6]+", 0x"+h[6:8]+", 0 ;"+str(i+index)+': '+str(freq))
