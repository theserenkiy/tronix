;chip m48
.include "C:\avr\includes\m48def.inc"


RESET:
	sbi DDRB,5
	cbi DDRD,5

MAIN:

	in r16,PIND
	out PORTB,r16


;	rcall DELAY_100us
;	sbi PORTD,6
;	rcall DELAY
;	cbi PORTD,6
	rjmp MAIN

DELAY:
	ldi r16,0
_loop:
	dec r16
	brne _loop
	ret

DELAY_100us:
	ldi r16,0
	ldi r17,2
_loop:
	dec r16
	brne _loop
	dec r17
	brne _loop
	ret
