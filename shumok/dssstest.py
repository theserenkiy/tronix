from m_sequence import gen_sequence
from dsss import *

input_data_str = "110100010101"
seq_power = 4


input_data = [int(x) for x in list(input_data_str)]


seq = gen_sequence(4,0x21324)



signal = spread(input_data, seq, span_chips=200)
signal = interpolate(signal,10,4)
# signal = add_noize(signal,3)

seq = [[],[1,0,1]]
signal = [1,1,0,0,1,1,1,1,0,0,1,1] #[1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8]

signal = find(signal, seq[1], 2)

print(signal)

# for ss in range(2):
# 	for i, s in enumerate(signal[ss]):
# 		print(f"{ss} {i} {sum([abs(x) for x in s])}")

# print(signal)


