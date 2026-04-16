polynoms = [
	None,
	None,
	None,
	None,
	(4,1),
	(5,2),
	(6,1),
	(7,3),
	(8,4,3,2),
	(9,4),
	(10,3),
	(11,2),
	(12,6,4,1)
]

def gen_sequence(power,seed):
	global polynoms

	if power >= len(polynoms) or not polynoms[power]:
		raise Exception(f"No polynoms for power {power}")

	seqlen = (1 << power) - 1
	polynom = polynoms[power]

	seq_p = []
	seq_n = []
	reg = seed
	for i in range(seqlen):
		bit = reg & 1
		seq_p.append(1 if bit else 0)
		seq_n.append(0 if bit else 1)
		temp = 0
		for p in polynom:
			temp ^= reg >> (p-1)
		reg = (reg >> 1) | ((temp & 1) << (power-1))

	return seq_n, seq_p




