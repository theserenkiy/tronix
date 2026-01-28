import re
import json

target_range_mhz = [144, 146]
# ifreqs_valid = [0.455, 2, 3, 6, 10.7]
ifreqs_valid = [0.5]
ifr_tolerance = 0.7 # 0.001
max_harmnum = 50



het_range_mhz = [target_range_mhz[0]-max(ifreqs_valid), target_range_mhz[1]+max(ifreqs_valid)]


def additem(d,floatkey,data):
	rkey = round(floatkey,3)
	skey = str(rkey)
	if skey not in d:
		d[skey] = {"key":rkey,"list":[]}
	if data not in d[skey]["list"]:
		d[skey]["list"].append(data)

def dump(d):
	print(json.dumps(d,indent="\t"))

def dumplist(l):
	dump(sorted(l.values(), key=lambda x: x["key"]))

f = open("list.txt","r",encoding="utf-8")
s = f.read()
f.close()

items = []

for line in s.split("\n"):
	mm = re.findall(r"^([\d\.]+)\s*([км])гц", line, flags=re.IGNORECASE)
	if mm:
		freq = float(mm[0][0])
		freq *= {"к":1e-3, "м":1}[mm[0][1].lower()]
		item = {"freq": freq}
		items.append(item)

	if "quantity" not in item:
		mm = re.findall(r"([\d]+)\s*шт.",line)
		if mm:
			item["quan"] = int(mm[0])
	
	if "price" not in item:
		mm = re.findall(r"([\d\.]+)\s*руб.",line)
		if mm:
			item["price"] = float(mm[0])
	
	mm = re.findall(r"Корпус\: (.+)",line)
	if mm:
		item["pkg"] = mm[0]
		item["is_hc49"] = 1 if mm[0].lower().startswith("hc-49") else 0


harms = {}
for item in items:
	is_good = 0
	item["harms"] = []
	item["is_good"] = 0
	for hnum in range(1,max_harmnum):
		harm = item["freq"]*hnum
		if het_range_mhz[0] <= harm and harm <= het_range_mhz[1]:
			rharm = round(harm,3)
			item["is_good"] = 1
			item["harms"].append(rharm)
			additem(harms,rharm,item["freq"])

gooditems = [x for x in items if x["is_good"]]

# print([x for x in items if x["is_good"]==1])

# harmlist = [float(x) for x in harms.keys()]

harmlist = sorted(harms.values(), key=lambda x: x["key"])
iflist = {}
iflist_harm = {}

for i in gooditems:
	i["iflist"] = []
	for ih in i["harms"]:
		for h in harmlist:
			if not (target_range_mhz[0] <= ih <= target_range_mhz[1]):
				# and not (target_range_mhz[0] <= h["key"] <= target_range_mhz[1]):
				continue
			ifr = round(abs(ih-h["key"]),3)
			if ifr:
				# for ifv in ifreqs_valid:
				# 	if ifv*(1-ifr_tolerance) < ifr < ifv*(1+ifr_tolerance):
				i["iflist"].append(ifr)
				additem(iflist,ifr,{
					"IF": ifr, 
					"harm_1": ih, 
					"xtal_freq_1": i["freq"],
					"harm_2": h["key"], 
					"xtal_freqs_2": h["list"]
				})



# range_freqs = {}
# for ifx in iflist.values():
# 	for item in ifx["list"]:
# 		additem(range_freqs, item["harm_1"], item["harm_2"])

# dumplist(range_freqs)

dump([x["freq"] for x in gooditems])