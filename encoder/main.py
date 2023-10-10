import machine
from machine import Pin, SoftI2C
import time


dat = Pin(17,Pin.IN,Pin.PULL_UP)
rises = 0
def encoder_irq(pin):
	global rises
	rises+=1


clk = Pin(18,Pin.IN,Pin.PULL_UP)
clk.irq(trigger=Pin.IRQ_FALLING,handler=encoder_irq)


while True:
	# values = []
	# for i in range(16):
	# 	values.append(clk.value())
	print(rises)
	q = input('Press enter')
	