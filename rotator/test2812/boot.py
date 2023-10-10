import machine
from machine import Pin
import time
import socket
from image import img

machine.freq(240000000)



def hue2color(v):
    v = v%360
    r = 120-abs(v-120)
    g = 120-abs(v-240)
    b = 120-abs(v-360) if v > 120 else 120-v;
    return [0 if v < 0 else v*2 for v in [r,g,b]]



def getColorLine(col):
    data = []
    for i in range(16):
        data += col
    return bytearray(data)

cnt = 0

data = [
    getColorLine([0,255,0]),
    getColorLine([0,0,0])
]


slices = []
cur_slice = bytearray([])
cnt=0
imlen = len(img)
huestep = 300/imlen
hue = 360+50
for pix in img:
    #cur_slice += bytearray(hue2color(int(hue)) if pix else [0,0,0])
    cur_slice += bytearray([0,255,0] if pix else [0,0,0])
    cnt+=1
    hue-=huestep
    if not cnt%16:
        slices.append(cur_slice)
        cur_slice = bytearray([])

# del img

#slices = 

slices.append(getColorLine([0,0,0]))

pin_sync1 = machine.Pin(32, machine.Pin.IN, machine.Pin.PULL_DOWN)
pin_sync2 = machine.Pin(25, machine.Pin.IN, machine.Pin.PULL_DOWN)
pin_sync3 = machine.Pin(33, machine.Pin.IN, machine.Pin.PULL_DOWN)


sync = 0
last_sync_irq = 0
last_sync = 0
def syncIrq1(pin):
    global sync,last_sync,last_sync_irq,pin_sync1,pin_sync2

    #print('irq',pin_sync1.value(),pin_sync2.value())
    now = time.time_ns()
    # tdiff = now-last_sync_irq
    # if tdiff < 40000000 and tdiff > 5000000:
    #     last_sync_irq = now
    #     return

    if pin_sync2.value()==pin_sync1.value():
        return

    last_sync_irq = now
    last_sync = now
    sync = 1

syncIrq1('хуй')


pin_sync1.irq(trigger=Pin.IRQ_RISING, handler=syncIrq1)

print('OK, lets go!')

bs_pin = Pin(15,Pin.OUT)
bs_timings = (400, 850, 800, 450)

delay_us = 0
while True:
    if not sync:
        continue
    
    sync = 0

    for sl in slices:
        machine.bitstream(bs_pin, 0, bs_timings, sl)
    
    if last_sync:
        tdiff = time.time_ns()-last_sync
        if tdiff < 10000000:
            delay_us -= 100
    else:
        delay_us += 100

    time.sleep_us(delay_us)

    endtime = time.time()
    
