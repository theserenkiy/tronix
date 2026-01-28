

class Canvas:
	def __init__(self,w,h):
		self.w = w
		self.h = h
		self.size = w*h
		self.buf = [[0,0,0] for x in range(self.size)]

	def putPixel(self,color,x,y):
		n = y*self.w + x
		self.buf[n] = color

	def setFont(self,font):
		self.font = font
		charsz = {}

	def sendToNeopixel(self,np):
		for n in range(64):
			np[n] = self.buf[n]
		np.write()

	def drawChar(self,char,color):
		x0 = self.x
		y0 = self.y
		if char not in self.font["refs"]:
			self.x += 5
			return None
		if x0 > self.w or y0 > self.h:
			return None
		gl = self.font["glyphs"][self.font["refs"][char]]
		glw = gl[0]
		gcode = gl[1]
		if x0+glw < 0:
			return None

		glh = int(len(gcode)/glw)
		if y0+glh < 0:
			return None
		 
		gx0 = 0 if x0 >= 0 else -x0
		gy0 = 0 if y0 >= 0 else -y0
		gx1 = glw if x0+glw < self.w else self.w-x0
		gy1 = glh if y0+glh < self.h else self.h-y0
		n = gy0*glw
		for gy in range(gy0,gy1):
			n+=gx0
			for gx in range(gx0,gx1):
				self.putPixel(color if gcode[n] else [0,0,0],gx+x0,gy+y0)
				n+=1
	

	


class ScrollingLine:

	def __init__(self,msg,font):
		self.msg = msg
		self.font = font
		self.pos_px = 0
		self.pos_char = 0
		self.offsets = [0]
		for c in msg:
			ref = font["refs"][c] if c in font["refs"] else -1
			self.addOffset(5 if ref < 0 else font["glyphs"][ref][0])

	def addOffset(self, n):
		self.offsets.append(self.offsets[-1]+n)

	def putOnCanvas(self,canvas,x,y):
		self.canvas = canvas

	def step(self):
		self.pos_px += 1
		if self.offsets[self.pos_char+1] <= self.pos_px:
			self.pos_char+=1

		for c in range(self.pos_char):




	
		
	