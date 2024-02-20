;chip m328p
.include "C:\avr\includes\m48def.inc"

	sbi DDRC,5

MAIN:
	sbi PORTC,5
	rcall DELAY
	cbi PORTC,5
	rcall DELAY
	rjmp MAIN


DELAY:
	ldi r16,0
	ldi r17,0
	ldi r18,20
_loop:
	dec r16
	brne _loop
	dec r17
	brne _loop
	dec r18
	brne _loop
	ret
