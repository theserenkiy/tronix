import sys
import os
from os import path
from subprocess import call

includepaths = ['C:/avr/avrasm/includes','C:/avr/lib']

#fuses = ''
#fuses = '-U lfuse:w:0xde:m -U hfuse:w:0xdf:m -U efuse:w:0xff:m'	#t2313
#fuses = '-U lfuse:w:0xDF:m -U hfuse:w:0xDF:m -U efuse:w:0xFF:m -U lock:w:0xFF:m '

asm_name = sys.argv[1]
dirname = path.dirname(asm_name)
_dir = path.dirname(__file__)

print('Dirname',_dir)

if dirname:
	os.chdir(dirname)
	asm_name = path.basename(asm_name)

name = path.splitext(asm_name)[0]
hexname = name + '.hex'
cntname = name + '.cnt'

if path.isfile(hexname):
	os.remove(hexname)

chipinfo = open(asm_name,'r').readline()
if chipinfo[:6] != ';chip ':
	print('Missing chip declaration')
	sys.exit

chip = chipinfo[5:].strip()

print("CHIP: "+chip)
print("############################################")
print("#               COMPILATION                #")
print("############################################")

incpaths = ' -I '.join(includepaths).replace('/','\\')

#cmd = _dir+'/avra -I '+incpaths+' '+asm_name
cmd = f'c:/avr/tavrasm/Tavrasmw.exe -i -v -x {name}.asm -o {hexname}'

#cmd = _dir+'/avrasm2 -fI -1 -I '+incpaths+' '+asm_name

print('Command: '+cmd)

result = call(cmd)

print(f'Result {result}')

if result !=0:
	print("\nCompilation failed\n")
	sys.exit()
else:
	print("\nCompiled successfully!!!\n")


if len(sys.argv) > 2 and sys.argv[2]=='prog':
	print("############################################")
	print("#               PROGRAMMING                #")
	print("############################################")
	cmd = _dir+'/avrdude/avrdude -p '+chip+' -c usbasp -B 50 -i 50 -U flash:w:"'+name+'.hex"'
	print('Command: '+cmd)

	result = call(cmd)
	print(f'Result {result}')

	if result != 0:
		print('\nProgramming failed :(\n')
	else:
		cnt = 1
		if not path.isfile(name+'.cnt'):
			fp = open(name+'.cnt','w')
		else:
			fp = open(name+'.cnt','r+')
			cnt = int(fp.read())
			cnt = cnt+1
			fp.truncate(0)
			fp.seek(0)

		fp.write(str(cnt))
		fp.close()
		flash_bytes = round((34/43/2)*(os.stat(name+'.hex').st_size-15))
		print('\nProgrammed successfully [bytes written: ',flash_bytes,'; prog count: ',cnt,']\n')
