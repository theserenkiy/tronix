
leds = 8
supply = 3
dynrange_db = 20
dynrange_volts = 0.5
total_resistance = 100000


upper_resistance = total_resistance*(dynrange_volts/(supply/2))
lower_resistance = total_resistance-upper_resistance

pow_step = (dynrange_db/(leds-1))/10

print("upper resistance: ",upper_resistance)
print("pow step: ",pow_step)

values = []
resistors = []

rest_res = upper_resistance
prev_val = 0
for i in range(leds):
	v = pow(10,-(pow_step*i))
	values.append(v)
	if prev_val:
		resistors.append((prev_val-v)*upper_resistance)
	prev_val = v
print(values)
print(resistors)

