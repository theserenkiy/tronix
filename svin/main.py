from machine import Pin, DAC, Timer, PWM, ADC
import math
import time
import random
import gc

gc.enable()

# DAC pin definitions (GPIO25 and GPIO26)
dac_pin = Pin(12,Pin.OUT,drive=Pin.DRIVE_3) # Or Pin(26) for the other DAC channel
# dac = DAC(dac_pin)
dac_pwm = PWM(dac_pin, freq=64000, duty_u16=32768)
dac_tim = Timer(0)

led_pin = Pin(25,Pin.OUT,drive=Pin.DRIVE_2)
led_pwm = PWM(led_pin, freq=20000, duty_u16=32768)
led_tim = Timer(1)

adc_pin = Pin(33)
adc = ADC(adc_pin)




buf = None
buf_pos = 0
buf_done = 1
buf_len = 0
play_exp = 0

volume = 6


led_rand = []
v = 32768
prev = v
steps = 4
for i in range(256):
    v = random.randint(20000,65535)
    step = int((v-prev)/steps)
    for k in range(steps):
        led_rand.append(prev+step*k)
    prev = v
    # if v < 10:
    #     v = 10
    # elif v > 15:
    #     v = 15
    

# print(led_rand)

led_pos = 0

def write_led(t):
    global led_rand,led_pos
    led_pwm.duty_u16(led_rand[led_pos])
    led_pos = (led_pos+1) & 0x3ff

led_tim.init(mode=Timer.PERIODIC, freq=30, callback=write_led)

def write_dac(t):
    global buf_done, buf_len, buf_pos, buf
    if buf_done:
        return
    # dac.write(buf[buf_pos])
    dac_pwm.duty_u16(buf[buf_pos] << 8)
    buf_pos += 1
    if buf_pos >= buf_len:
        dac_pwm.duty_u16(32768)
        buf_done = 1
        dac_tim.deinit()
        buf = None
        gc.collect()
    
def play_file(name):
    global buf,buf_len,buf_done,buf_pos,dac_tim,play_started
    print("Play file: "+name)
    play_started = time.time()
    f = open(name,"rb")
    buf = bytearray(f.read())
    buf_len = len(buf)
    f.close()

    buf_done = 0
    buf_pos = 0

    dac_tim.init(mode=Timer.PERIODIC, freq=8000, callback=write_dac)


good_files = [1,2,3,4,6,7,8]

next_play = 0
while True:
    if time.time() > next_play:
        next_play = time.time()+random.randint(30,500)
        print("Time: ",time.time()," Next play:",next_play)
    # if buf_done:
        r = random.choice(good_files)
        # r = 4
        play_file("svin"+str(r)+".raw")
    
    volume = 1+int(adc.read_u16()/8192)
    time.sleep_ms(250)