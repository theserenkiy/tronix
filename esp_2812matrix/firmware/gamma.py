
power = 2.5
bits = 7
values = 2**bits
max = (10**power)/values
gamma = [round(10**(i*power/values)/max) for i in range(values)]

print(gamma)