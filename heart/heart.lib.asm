.include "heart.macros.asm"

.equ PIN_LED = 4
.equ PIN_RF = 0
.equ PIN_AREF = 1
.equ PIN_DIVIDER = 3


.cseg
.org 0
	rjmp RESET
;.org 0x0005
;	rjmp IRQ_ANACOMP
.org 0x0006
	rjmp IRQ_COMPA
.org 0x0008
	rjmp IRQ_WDOG

.org 0x000A

COMMON_SETTINGS:
	cbi DDRB,PIN_AREF
	sbi DDRB,PIN_LED
	cbi DDRB,PIN_DIVIDER

	;sleep mode = POWER DOWN
	outi MCUCR,(1 << SE | 1 << SM1 | 0 << SM0)

	;configure watchdog
	outi WDTCR, WDOG_CFG

	;configure Timer
	outi TCCR0A,0b10	; CTC
	outi TIMSK0,0b100	; COMPA IRQ


	sei
	ret


DEADLOOP:
	rjmp PC

IRQ_COMPA:
	ldi r24,0
	reti

START_TIMER:	;r16 - comparator
	cli
	outi TCCR0B,0b011	; T/64
	outi TCNT0,0
	out OCR0A,r16
	ldi r24,1
	sei
	ret

STOP_TIMER:
	outi TCCR0B,0b010	; parking timer
	outi TCNT0,0
	ret


RX_NOISE_REDUCTED:	;exec while r24!=0, return - r17
	drain_cap
	acomp_on 0
	;ldi r16,10	;cycle counter
	ldi r17,0	;result
	ldi r18,0xff	;shift register

_read_loop:
	clc
	sbic ACSR, ACO
	sec
	rol r18

	mov r19,r18
	andi r19,0b00010001
	brne _cont
	mov r19,r18
	andi r19,0b00001000
	brne _ok
_cont:
	tst r24
	brne _read_loop
	ret
_ok:
	ldi r17,1
	;drain_cap
	ret



BEAT:
	sbi PORTB,PIN_LED
	ldi r16,20
	rcall DELAY
	cbi PORTB,PIN_LED
	ret

DBEAT:
	rcall BEAT
	ldi r16,0
	rcall DELAY
	rcall DELAY
	rcall DELAY
	rcall BEAT
	ret

TX_PULSE:
	sbi DDRB,0
	sbi DDRB,PIN_RF
	sbi PORTB,PIN_RF
	nop
	nop
	nop
	nop

	;ldi r16,0
	;rcall DELAY
	cbi PORTB,PIN_RF	;discharge capacitors
	cbi DDRB,PIN_RF
	ret

DELAY:
	;ldi r16,0
_loop:
	dec r16
	brne _loop
	ret

DELAY2:
	ldi r17,0
_loop:
	dec r17
	brne _loop
	dec r16
	brne _loop
	ret
