import serial
import numpy as np
import matplotlib.pyplot as plt

N = 15000

print("Connecting....")

ser = serial.Serial("COM4", 115200)

print("Connected! Reading")

raw = ser.read(N * 2)

data = np.frombuffer(raw, dtype=np.uint16)

plt.figure(figsize=(14, 8))
plt.plot(data)

plt.grid(True)
plt.tight_layout()

plt.show()