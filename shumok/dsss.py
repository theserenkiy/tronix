import random

def spread(data, sequence, span_chips=0):
	out = [0]*span_chips
	polarseq = [[1 if x else -1 for x in sequence[n]] for n in range(2)]
	for v in data:
		out += polarseq[v]
	return out


def interpolate(sig, scale, fade_steps):
	state = 0
	out = [0]
	for v in sig:
		dif = v-state
		step = dif/fade_steps
		for snum in range(fade_steps):
			state += step
			out.append(state)
		out += [float(v)]*(scale-fade_steps)
	return out


def add_noize(sig, ampl):
	pp = ampl*2
	return [x + pp*(random.random()-0.5) for x in sig]




def find(sig, seq_p, chip_len):
	seqlen = len(seq_p)
	seqlen_x2 = seqlen*2
	accums = [[0]*seqlen for _ in range(2)]
	out = [[[].copy()]*seqlen for _ in range(2)]
	sums = []
	step = 0

	print("OUT",out)

	chiplen_h = int(chip_len/2)
	avgsig = [sum(sig[x:x+chiplen_h]) for x in range(0,len(sig),chiplen_h)]

	print(f"AVGsig {avgsig}")

	start_stream = 0
	for i in range(len(avgsig)):
		n_substream = 1 if i % 2 else 0
		acc = accums[n_substream]
		v = [-avgsig[i],avgsig[i]]
		
		out[n_substream][start_stream].append(acc[start_stream])
		acc[start_stream] = 0

		for nbit in range(seqlen):
			bit = seq_p[nbit]
			accn = (start_stream-nbit) % +seqlen
			acc[accn] += v[bit]
			print(f"Acc {n_substream}.{accn} + {v[bit]}")

		if i % 2:
			start_stream = (start_stream+1) % seqlen
		pass
	return out