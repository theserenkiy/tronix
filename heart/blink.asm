;chip t13
.include "tn13def.inc"

ldi r16,0xFF
out ddrb,r16
out portb,r16

MAIN:
	sbi portb,4
	rcall DELAY
	cbi portb,4
	rcall DELAY
	rjmp MAIN


DELAY:
	ldi r16,0
	ldi r17,0
_loop:
	dec r16
	brne _loop
	dec r17
	brne _loop
	ret
