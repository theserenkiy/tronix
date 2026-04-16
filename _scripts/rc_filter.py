
F = 10000
C = 1e-9
R = 0

px2 = 6.283

foo = {
	"R": [R,lambda: 1/(px2 * F * C),"кОм",1e-3],
	"C": [C,lambda: 1/(px2 * F * R),"нФ",1e9],
	"F": [F,lambda: 1/(px2 * R * C),"кГц",1e-3]
}

for x in foo:
	tocalc = not foo[x][0]
	v = (foo[x][1]() if tocalc else foo[x][0])*foo[x][3]
	print(f"{x} = ",round(v,3),foo[x][2],"<--" if tocalc else "")
