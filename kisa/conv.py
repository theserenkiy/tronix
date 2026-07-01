import sys
import os

if len(sys.argv) < 2:
	exit("Missing name")

name = sys.argv[1]
cmd = f"ffmpeg -y -i {name}.ogg -c:a pcm_s16le -ac 1 -ar 44100 {name}.wav"

print(cmd)
os.system(cmd)