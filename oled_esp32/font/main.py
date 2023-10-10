import machine
from machine import Pin, SoftI2C
import time
import math
from font16 import font

letter_spacing = 5

#i2c = SoftI2C(scl=Pin(22), sda=Pin(21), freq=100000)	#Dane4ka
i2c = SoftI2C(scl=Pin(22), sda=Pin(23), freq=100000)	#Serega
print('Found I2C devices:',i2c.scan())

def cmd(cmd):
	if not isinstance(cmd,list):
		cmd = [cmd]

	for reg in cmd:
		i2c.writeto(60,bytearray([0b10000000,reg]))

def init():
	cmd(0xAE)			#Display OFF
	cmd([0x20,0x00])	#Page addressing mode;  00 - horiz, 01 - vert, 10 - page
	cmd(0xB0)			#page address: 0 (B0...B7)
	cmd(0xC8)			#piece of bullshit, go fuck
	cmd(0x00)			#Lower Column Start Address for Page Addressing Mode                
	cmd(0x10)			#Higher Column Start Address for Page Addressing Mode                
	cmd([0x22,0,7])		
	cmd([0x21,0,127])
	cmd(0x40)			#RAM display start line register (0-63)  0b01xxxxxx
	cmd([0x81,0xff])	#contrast
	cmd(0xA1)			#Segment Re-map  0b1010000x
	cmd(0xA6)			#normal/inverse 0b1010011X   0 - normal, 1 - inverse
	cmd([0xA8,0x3F])	#another piece of bullshit, suck my dick
	cmd(0xA4)			#Output follows(A4)/ignores(A5) RAM content
	cmd([0xD3,0])		#Set Display Offset (0-63)
	cmd([0xD5,0xF0])	#set freq & divide ratio: A[3:0] - ratio, A[7:4] - freq
	cmd([0xD9,0x22])	#Set Pre-charge Period (unknown fucking bullshit)
	cmd([0xDA,0x12])	#Set COM Pins Hardware Configuration
	cmd([0xDB,0x20])	#VCOMH Deselect Level 	
	cmd([0x8D,0x14])	#Charge Pump Setting A=0b00010X00   0 - disable, 1 - enable
	cmd(0xAF)			#Display ON 

cursor = [0,0]

def clearDisplay():
	cmd([0x22,0,7])		
	cmd([0x21,0,127])
	i2c.writeto(60,bytearray([0b01000000]+[0 for x in range(1024)]))


def setCursor(col,row):
	global cursor
	cursor = [col,row]	
	checkCursor(cursor)


def checkCursor(cursor):
	if cursor[0] < 0:
		cursor[0] = 0
	if cursor[0] > 127:
		cursor[0] = 127
	if cursor[1] < 0:
		cursor[1] = 0
	if cursor[1] > 7:
		cursor[1] = 7


def printStr(str,spacing=1,align=0,monospace=0):
	space = [0 for x in range(spacing)]
	image = []
	maxw = 0
	if monospace:
		for char in str:
			charw = int(len(font['glyphs'][char])/font['height_bytes'])
			if charw > maxw:
				maxw = charw
	#print('maxw ',maxw)
	for b in range(font['height_bytes']):
		for char in str:
			glyph = font['glyphs'][char]
			glyphWidth = int(len(glyph)/font['height_bytes'])
			startByte = b*glyphWidth
			endByte = startByte+glyphWidth
			xspace = []
			if monospace and glyphWidth < maxw:
				xspace = [0 for x in range(maxw-glyphWidth)]
				
			image += glyph[startByte:endByte]+xspace+space

	strWidth = int(len(image)/font['height_bytes'])
	drawToArea(image,strWidth,font['height_bytes'],align=align)



def drawToArea(bytes,w,h,align=0):
	cur = []+cursor
	if align==1:
		cur[0]-=w
	elif align==2:
		cur[0] -= math.ceil(w/2)
	
	checkCursor(cur)

	#print('cursor:',cursor)
	endrow = cur[1]+h-1
	endcol = cur[0]+w-1
	if endrow > 7:
		endrow = 7
	if endcol > 127:
		endcol = 127
	cmd([0x22,cur[1],endrow])
	cmd([0x21,cur[0],endcol])
	i2c.writeto(60,bytearray([0b01000000]+bytes))
	


def fillArea(val,w,h):
	return drawToArea([val for x in range(w*h)],w,h)

init()

clearDisplay()

setCursor(20,3)
#printStr('1234',spacing=5,align=2)



cnt = 0
while True:
	printStr(str(cnt),spacing=4,align=0)
	cnt+=1
	time.sleep_ms(100)

 