#fridge motherboard emulator
import machine
from machine import Pin, SoftI2C
import time

hi = 0
lo = 1

sig = Pin(2,Pin.OUT)

def signal(delay,duration):
	while True:
		sig.value(lo)
		time.sleep_ms(delay)
		sig.value(hi)
		time.sleep_ms(delay)
		duration -= delay*2
		if duration <= 0:
			break

while True:
	for i in [9,25]:
		sig.value(hi)
		time.sleep_ms(1500)
		signal(i,250)


