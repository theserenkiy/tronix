
f = 145e6
output_imp = 37
q = 3.5
vcc = 3.5
desired_power = 0.1

def c(xc):
	return round(1e12/(2 * pi * f * xc),1)

input_imp = vcc**2/(2*desired_power)

ro = output_imp
ri = input_imp
xco = ro/q
xci = ri*((ro/ri)/((q**2 + 1)-ro/ri))**0.5
xl = (q*ro + ((ro*ri) / xci)) / (q**2 + 1)
pi = 3.1415
ci = c(xci)
co = c(xco)
l = round((1e6 * xl) / (2 * pi * f),3)


print("Inp impedance = ",ri)
print("C inp (pF) = ",ci)
print("C out (pF) = ",co)
print("L (uH) = ",l)