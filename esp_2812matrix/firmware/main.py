import machine, neopixel, json, usocket, network, time
import lib
from font import FONT_font2 as font
import canvas
# j = "[[255,0,0],[0,255,0],[0,0,255],[255,255,0],[255,0,255],[0,255,255]]"
# d = json.loads(j)

print("STARTING...")

SERVER_HOST = '37.46.135.97'
SERVER_PORT = 80 #3210

COLOR_DIV = [1,1.5,2]


lib.init_wlan('Qqq', 'huivam!!')

j = ""
while True:
	j = lib.fetch_socket(SERVER_HOST,SERVER_PORT,"aaa")
	if j:
		d = [[round(lib.GAMMA[c >> 1]/COLOR_DIV[k]) for k,c in enumerate(x)] for x in json.loads(j)]
		np = neopixel.NeoPixel(machine.Pin(13), 64)
		for n in range(64):
			np[n] = d[n]
		np.write()
	time.sleep(2)

# c = canvas.Canvas(8,8)
# c.setFont(font)
# c.drawChar("K",1,1,[0,0,10])
# c.drawString("ВСЕ",1,1,[0,0,10])

# c.sendToNeopixel(np)

