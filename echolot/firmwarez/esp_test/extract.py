from lib import *

fnames = getFilenamesFromArgs("tmp.bin")

with open(fnames[0],"rb") as f:
	data = extractRawData(f.read())
	with open("extracted.bin","wb") as f:
		f.write(data)