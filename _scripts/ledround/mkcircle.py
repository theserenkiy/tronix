import sys
import re
import math

print(sys.argv)

try:
	
	d = float(sys.argv[1])
	count = float(sys.argv[2])
	refdes = sys.argv[3].lower()
	file = sys.argv[4]

	f = open(file)
	ll = f.read().split("\n")
	f.close()

	print(f"Lines: {len(ll)}")

	doc = {}
	lvl = 0
	in_component = 0
	angstep = 360/count 
	compcnt = -1
	r = d/2
	for i in range(len(ll)):
		l = ll[i].rstrip()

		mm = re.match(r"^\s*",l)

		lvl = int(len(mm.group(0))/2)
		# print(lvl)

		if not in_component:
			if lvl < 2:
				continue
			mm = re.search(r"\(Component\s+[^\s]+\s+([a-zA-Z]+)\d+",l)  #  ([^\)]+)
			if not mm:
				continue

			rd = mm.group(1)
			if rd.lower() != refdes:
				continue

			in_component = 1
			compcnt += 1

			print(f"Component {mm.group(0)}")

		else:
			if lvl > 3:
				continue
			if lvl < 3:
				in_component = 0
				continue
			
			mm = re.search(r"\(([A-Za-z]+)\s+(\-?\d+)",l)
			if mm:
				name = mm.group(1)
				value = mm.group(2)
				if name=="Angle" or name=="X" or name=="Y":
					print("\t",name,value)
					val = ''
					if name=="X":
						val = math.sin(angstep*compcnt)
					elif name=="X":
						val = math.cos(angstep*compcnt)
					else:
						val = angstep*compcnt
					ll[i] = ('  '*lvl)+f"({name} {val})"
				
			
	open('out.asc','w').write("\n".join(ll))




except Exception as e:
	print(e)
	print("\nUsage: mkcircle.py <diameter,mm> <sectors count> <refdes prefix (i.e VD)> <filename.asc>")