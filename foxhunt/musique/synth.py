import pyaudio
import numpy as np
import math
import re
import time

# Параметры звука
BITRATE = 16000  # Частота дискретизации (Гц)
FREQUENCY = 440  # Частота ноты Ля (Гц)
LENGTH = 2       # Длительность (секунды)

BASE_LEN = 0.2

p = pyaudio.PyAudio()

start = 440/(math.pow(2,9/12))
note_freq = {}
num = 0
for oct in range(3):
	for note in ["do","do#","re","re#","mi","fa","fa#","sol","sol#","la","la#","si"]:
		note_freq[note+("" if not oct else str(oct+1))] = start*math.pow(2,num/12)
		num += 1

stream = None

def mkWave(freq: float, leng: float):
	# Генерация синусоидальной волны
	return (np.sign(np.sin(2 * np.pi * np.arange(BITRATE * leng) * freq / BITRATE))).astype(np.float32)

def playWave(wave):
	global stream
	if not stream:
		stream = p.open(format=pyaudio.paFloat32,
                channels=1,
                rate=BITRATE,
                output=True)
	stream.write(wave.tobytes())

def playTone(freq: float, leng: float):
	playWave(mkWave(freq, leng))


def playNote(note: str, leng: float):
	freq = note_freq[note]
	playTone(freq, leng)


# for note in note_freq.keys():
# 	playNote(note, 0.5)

def readFile(filename):
	f = open(filename,"r",encoding="utf-8")
	s = f.read()
	f.close()

	subs = s.split(">")
	s = subs[len(subs)-1]

	lnum = 1
	for l in s.split("\n"):
		l = l.strip()
		if not l:
			continue
		try: 
			pp = re.split(r"\s+", l)
			leng = int(pp[1])*BASE_LEN
			duty = 0.7
			if pp[0] != "p":
				playNote(pp[0],leng*duty)
				time.sleep(leng*(1-duty))
			else:
				time.sleep(leng*duty)
			
		except Exception as e:
			print(f"ERROR on line {lnum} ({l}, {pp}): {e}")
		lnum += 1
	


readFile("poplanu.txt")
# readFile("malchik.txt")


stream.stop_stream()
stream.close()
p.terminate()

