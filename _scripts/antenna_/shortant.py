import sys
import math

f_mhz = float(sys.argv[1])
l_m = float(sys.argv[2])
d_mm = float(sys.argv[3])

print(f_mhz, l_m, d_mm)

hd = l_m/2
lamb = 300/f_mhz
Remis = 1600*((hd/lamb)**2)

d0 = d_mm * 0.001
We = 60*(math.log(2*l_m/d0-1))
k = 2*math.pi/lamb

Xa = We*(1/math.tan(k*l_m))
Ly = Xa/(2*math.pi*f_mhz)

print('Ly = ',Ly,'; R emis = ',Remis)