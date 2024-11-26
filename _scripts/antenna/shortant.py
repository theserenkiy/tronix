# Arguments: 
# 1: frequency, MHz
# 2: antenna length, m
# 3: antenna wire diameter, mm
# Source: V.A.Dnischenko Dist. upr. modelyami, p. 190

import sys
import math

freq_mhz = float(sys.argv[1])
leng_m = float(sys.argv[2])
diam_mm = float(sys.argv[3])


print('Freq: ',freq_mhz,'MHz; Length: ',leng_m,'m; Diam:',diam_mm,'mm')

hd = leng_m/2
lamb = 300/freq_mhz
R_emis = 1600 * (hd/lamb)**2

print('R emission = ',R_emis)

d0 = 0.001*diam_mm
We = 60*(math.log(2*leng_m/d0-1))
k = 2*math.pi/lamb

Xa = We*(1/math.tan(k*leng_m))

print("Xa",Xa,'Ohm')

Ly = Xa/(2*math.pi*freq_mhz*1e6)
Ly_uH = Ly*1e6

print('Ly, uH = ',Ly_uH)


# Xa = W
