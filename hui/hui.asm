;chip m48
.include "C:\avr\includes\m48def.inc"

.equ FOSC = 16000000

.macro outi
	ldi r25,@1
	out @0,r25
.endm
.macro stsi
	ldi r25,@1
	sts @0,r25
.endm

.dseg
.org 100
note: .byte 1

.cseg
	rjmp RESET
.org 0x00B
	rjmp T1_COMPARE

.org 0x20


RESET:
	ldi r16,0
	ldi r17,0
_loop:
	dec R16
	brne _loop
	dec r17
	brne _loop

	outi SMCR,0b1

	sbic MCUSR,0
	rjmp EXT_RESET
	ldi r20,0
	sleep

EXT_RESET:
	cbi MCUSR,0
	outi DDRD,0xff
	outi DDRB,0xff
	outi PORTB,0xff

	cpi r20,16
	brlo INIT_CNTS
	ldi r20,0

INIT_CNTS:
	stsi TCCR1A,0
	stsi TCCR1B,0b00001011
	stsi TIMSK1,1 << OCIE1A

	ldi ZH,high(NOTES*2)
	ldi ZL,low(NOTES*2)
	ldi r17,0
	add ZL,r20
	adc ZH,r17
	lpm r17,Z
	stsi OCR1AH,0
	sts OCR1AL,r17

	inc r20

	outi TCCR0A,0b10000011
	outi TCCR0B,0b00000001
	outi OCR0A,1

	ldi ZH,high(HUI*2)
	ldi ZL,low(HUI*2)

	sei

MAIN:
	rjmp MAIN

T1_COMPARE:
	lpm r16,Z+
	tst r16
	brne _play
	outi OCR0A,1
	rjmp PC
_play:
	out OCR0A,r16
_end:
	reti

NOTES:
.db FOSC/64/7000, FOSC/64/7856, FOSC/64/8819, FOSC/64/9340, FOSC/64/10483, FOSC/64/11767, FOSC/64/13211, FOSC/64/13993
.db FOSC/64/13993, FOSC/64/13211, FOSC/64/11767, FOSC/64/10483, FOSC/64/9340, FOSC/64/8819, FOSC/64/7856, FOSC/64/7000

HUI:
.db 0x1, 0x5, 0x9, 0xd, 0x11, 0x15, 0x19, 0x1d, 0x21, 0x25, 0x29, 0x2d, 0x31, 0x35, 0x39, 0x3d
.db 0x41, 0x45, 0x49, 0x4d, 0x51, 0x55, 0x59, 0x5d, 0x61, 0x65, 0x69, 0x6d, 0x71, 0x75, 0x79, 0x7d
.db 0x80, 0x81, 0x80, 0x80, 0x80, 0x80, 0x7f, 0x7f, 0x7d, 0x7c, 0x7c, 0x7d, 0x7e, 0x80, 0x82, 0x83
.db 0x83, 0x81, 0x7f, 0x7e, 0x7e, 0x7f, 0x81, 0x83, 0x84, 0x83, 0x80, 0x7e, 0x7d, 0x7d, 0x7d, 0x7d
.db 0x7e, 0x7e, 0x7f, 0x80, 0x82, 0x83, 0x83, 0x84, 0x83, 0x81, 0x7f, 0x7b, 0x78, 0x76, 0x77, 0x7a
.db 0x7f, 0x83, 0x85, 0x85, 0x83, 0x82, 0x81, 0x80, 0x81, 0x82, 0x82, 0x82, 0x81, 0x80, 0x7e, 0x7d
.db 0x7d, 0x7e, 0x7f, 0x80, 0x7f, 0x7e, 0x7c, 0x7d, 0x80, 0x85, 0x89, 0x8a, 0x87, 0x80, 0x7a, 0x76
.db 0x74, 0x77, 0x7c, 0x81, 0x84, 0x84, 0x82, 0x7e, 0x7c, 0x7d, 0x80, 0x85, 0x89, 0x88, 0x83, 0x79
.db 0x70, 0x6c, 0x6f, 0x79, 0x86, 0x90, 0x91, 0x8b, 0x81, 0x77, 0x74, 0x77, 0x7f, 0x88, 0x8d, 0x8d
.db 0x87, 0x7e, 0x75, 0x6f, 0x71, 0x77, 0x81, 0x89, 0x8d, 0x8c, 0x87, 0x80, 0x7b, 0x79, 0x7a, 0x7d
.db 0x80, 0x81, 0x7f, 0x7c, 0x7a, 0x79, 0x7c, 0x81, 0x86, 0x89, 0x89, 0x86, 0x81, 0x7c, 0x79, 0x78
.db 0x79, 0x7a, 0x7b, 0x7c, 0x7e, 0x81, 0x85, 0x87, 0x88, 0x86, 0x84, 0x81, 0x7e, 0x7a, 0x78, 0x77
.db 0x78, 0x7d, 0x82, 0x86, 0x87, 0x84, 0x7f, 0x7b, 0x7b, 0x7f, 0x85, 0x89, 0x88, 0x83, 0x7a, 0x73
.db 0x6f, 0x72, 0x7a, 0x84, 0x8e, 0x93, 0x90, 0x87, 0x7a, 0x6e, 0x6b, 0x70, 0x7e, 0x8b, 0x96, 0x95
.db 0x8a, 0x7a, 0x6b, 0x64, 0x68, 0x76, 0x88, 0x98, 0x9d, 0x97, 0x87, 0x75, 0x69, 0x66, 0x6e, 0x7c
.db 0x8b, 0x93, 0x90, 0x87, 0x7b, 0x74, 0x71, 0x76, 0x7f, 0x8a, 0x91, 0x8f, 0x87, 0x7a, 0x6f, 0x6c
.db 0x72, 0x7e, 0x8d, 0x97, 0x99, 0x8f, 0x7d, 0x6a, 0x5f, 0x63, 0x73, 0x87, 0x97, 0x9d, 0x97, 0x86
.db 0x75, 0x69, 0x67, 0x70, 0x7e, 0x8c, 0x92, 0x90, 0x87, 0x7c, 0x74, 0x72, 0x78, 0x81, 0x89, 0x8d
.db 0x8b, 0x85, 0x7d, 0x79, 0x76, 0x78, 0x7c, 0x81, 0x84, 0x83, 0x82, 0x80, 0x81, 0x81, 0x81, 0x7f
.db 0x7e, 0x7d, 0x7c, 0x7c, 0x7c, 0x7e, 0x80, 0x83, 0x83, 0x82, 0x80, 0x7f, 0x7f, 0x80, 0x82, 0x81
.db 0x7f, 0x7d, 0x7c, 0x7e, 0x7f, 0x7f, 0x7f, 0x80, 0x81, 0x81, 0x7f, 0x7e, 0x7e, 0x80, 0x82, 0x82
.db 0x80, 0x7c, 0x7a, 0x78, 0x7a, 0x7d, 0x80, 0x82, 0x83, 0x83, 0x85, 0x86, 0x82, 0x7e, 0x79, 0x77
.db 0x79, 0x7f, 0x84, 0x8b, 0x8c, 0x88, 0x7f, 0x75, 0x70, 0x70, 0x78, 0x84, 0x91, 0x97, 0x95, 0x89
.db 0x77, 0x6a, 0x67, 0x6e, 0x7b, 0x88, 0x90, 0x91, 0x8d, 0x85, 0x7a, 0x71, 0x6e, 0x71, 0x7a, 0x85
.db 0x8d, 0x8c, 0x87, 0x7e, 0x76, 0x74, 0x77, 0x80, 0x87, 0x8b, 0x88, 0x81, 0x7a, 0x75, 0x75, 0x79
.db 0x82, 0x8a, 0x8d, 0x8a, 0x81, 0x78, 0x72, 0x71, 0x76, 0x80, 0x8b, 0x8f, 0x8d, 0x86, 0x78, 0x6f
.db 0x6c, 0x75, 0x80, 0x8c, 0x92, 0x90, 0x87, 0x7b, 0x71, 0x6e, 0x74, 0x81, 0x8e, 0x95, 0x92, 0x85
.db 0x75, 0x68, 0x66, 0x6d, 0x7c, 0x8d, 0x99, 0x9b, 0x92, 0x83, 0x71, 0x67, 0x67, 0x6f, 0x7d, 0x8b
.db 0x94, 0x93, 0x8a, 0x7e, 0x74, 0x6f, 0x73, 0x7b, 0x85, 0x8e, 0x90, 0x8c, 0x81, 0x78, 0x6f, 0x6d
.db 0x72, 0x7c, 0x89, 0x92, 0x94, 0x8e, 0x84, 0x78, 0x70, 0x6f, 0x74, 0x7c, 0x85, 0x8a, 0x8b, 0x87
.db 0x81, 0x7b, 0x77, 0x78, 0x7a, 0x7e, 0x83, 0x86, 0x87, 0x86, 0x82, 0x7e, 0x7a, 0x7a, 0x7b, 0x7d
.db 0x7f, 0x80, 0x81, 0x81, 0x82, 0x82, 0x82, 0x80, 0x7d, 0x7b, 0x7b, 0x7d, 0x81, 0x85, 0x84, 0x81
.db 0x7e, 0x7c, 0x7b, 0x7c, 0x7f, 0x80, 0x80, 0x7f, 0x7e, 0x7e, 0x7f, 0x80, 0x81, 0x82, 0x82, 0x83
.db 0x82, 0x81, 0x7d, 0x78, 0x76, 0x78, 0x7e, 0x85, 0x8c, 0x8c, 0x86, 0x7e, 0x76, 0x74, 0x77, 0x7d
.db 0x83, 0x87, 0x87, 0x83, 0x7f, 0x7b, 0x7a, 0x7c, 0x7f, 0x84, 0x85, 0x84, 0x80, 0x7c, 0x78, 0x77
.db 0x7c, 0x81, 0x87, 0x89, 0x88, 0x83, 0x7c, 0x76, 0x75, 0x7a, 0x81, 0x89, 0x8c, 0x8a, 0x84, 0x7d
.db 0x76, 0x71, 0x70, 0x74, 0x7e, 0x88, 0x92, 0x94, 0x90, 0x84, 0x78, 0x6f, 0x6c, 0x71, 0x7a, 0x84
.db 0x8c, 0x8f, 0x8c, 0x85, 0x7c, 0x73, 0x6f, 0x72, 0x7a, 0x87, 0x91, 0x94, 0x8e, 0x83, 0x76, 0x6c
.db 0x6b, 0x71, 0x7d, 0x88, 0x91, 0x92, 0x8f, 0x87, 0x7e, 0x75, 0x71, 0x71, 0x76, 0x7e, 0x85, 0x89
.db 0x88, 0x87, 0x82, 0x7d, 0x78, 0x77, 0x79, 0x7e, 0x83, 0x87, 0x89, 0x86, 0x82, 0x7d, 0x7b, 0x7b
.db 0x7d, 0x7f, 0x80, 0x81, 0x7f, 0x7e, 0x7d, 0x7c, 0x7e, 0x80, 0x82, 0x82, 0x81, 0x82, 0x82, 0x83
.db 0x81, 0x80, 0x7e, 0x7c, 0x7b, 0x7b, 0x7c, 0x7e, 0x7f, 0x81, 0x83, 0x85, 0x85, 0x83, 0x81, 0x7c
.db 0x7a, 0x7a, 0x7c, 0x7e, 0x7f, 0x7f, 0x7d, 0x7c, 0x7e, 0x81, 0x85, 0x87, 0x87, 0x84, 0x80, 0x7c
.db 0x79, 0x77, 0x79, 0x7b, 0x7f, 0x82, 0x83, 0x83, 0x81, 0x80, 0x7f, 0x80, 0x81, 0x83, 0x82, 0x81
.db 0x7d, 0x79, 0x77, 0x76, 0x79, 0x7d, 0x85, 0x8b, 0x8e, 0x8e, 0x87, 0x81, 0x79, 0x75, 0x77, 0x7a
.db 0x81, 0x85, 0x86, 0x84, 0x7f, 0x7b, 0x78, 0x79, 0x7e, 0x83, 0x86, 0x88, 0x86, 0x83, 0x7c, 0x75
.db 0x71, 0x72, 0x7a, 0x84, 0x8e, 0x92, 0x90, 0x88, 0x7c, 0x71, 0x6c, 0x6c, 0x74, 0x7c, 0x85, 0x8a
.db 0x8b, 0x8a, 0x86, 0x81, 0x7b, 0x77, 0x76, 0x77, 0x7c, 0x81, 0x85, 0x87, 0x87, 0x85, 0x82, 0x7d
.db 0x7b, 0x7c, 0x7f, 0x84, 0x86, 0x87, 0x86, 0x81, 0x7d, 0x79, 0x77, 0x78, 0x79, 0x7d, 0x7f, 0x83
.db 0x88, 0x8a, 0x8c, 0x89, 0x84, 0x7e, 0x78, 0x73, 0x71, 0x72, 0x76, 0x7b, 0x80, 0x86, 0x8b, 0x8e
.db 0x8f, 0x8b, 0x84, 0x7b, 0x75, 0x72, 0x72, 0x76, 0x7a, 0x81, 0x85, 0x89, 0x88, 0x85, 0x81, 0x7c
.db 0x79, 0x77, 0x7a, 0x7d, 0x82, 0x87, 0x88, 0x87, 0x83, 0x7e, 0x7b, 0x78, 0x79, 0x7a, 0x7b, 0x7d
.db 0x80, 0x83, 0x87, 0x88, 0x88, 0x86, 0x84, 0x82, 0x7f, 0x7e, 0x7d, 0x7c, 0x7c, 0x7a, 0x79, 0x77
.db 0x78, 0x7a, 0x7e, 0x83, 0x8a, 0x8d, 0x8e, 0x8a, 0x85, 0x7f, 0x77, 0x73, 0x70, 0x71, 0x76, 0x7e
.db 0x87, 0x8d, 0x90, 0x8e, 0x88, 0x7f, 0x78, 0x73, 0x74, 0x77, 0x7c, 0x82, 0x84, 0x83, 0x80, 0x7c
.db 0x79, 0x79, 0x7d, 0x83, 0x8b, 0x91, 0x93, 0x8e, 0x85, 0x79, 0x6e, 0x67, 0x66, 0x6c, 0x77, 0x83
.db 0x8e, 0x94, 0x94, 0x8f, 0x87, 0x7f, 0x79, 0x77, 0x77, 0x79, 0x7d, 0x80, 0x81, 0x80, 0x7e, 0x7a
.db 0x7a, 0x7a, 0x7d, 0x82, 0x87, 0x8b, 0x8c, 0x89, 0x84, 0x7d, 0x77, 0x73, 0x73, 0x76, 0x7b, 0x83
.db 0x87, 0x8a, 0x89, 0x85, 0x7f, 0x78, 0x74, 0x74, 0x78, 0x7e, 0x86, 0x8b, 0x8f, 0x8e, 0x89, 0x81
.db 0x79, 0x73, 0x70, 0x72, 0x77, 0x7d, 0x82, 0x87, 0x88, 0x88, 0x85, 0x81, 0x7d, 0x7b, 0x7b, 0x7d
.db 0x80, 0x83, 0x85, 0x84, 0x81, 0x7c, 0x78, 0x76, 0x78, 0x7c, 0x81, 0x86, 0x8a, 0x8b, 0x8a, 0x85
.db 0x7f, 0x79, 0x74, 0x73, 0x74, 0x79, 0x7f, 0x85, 0x89, 0x8b, 0x8b, 0x89, 0x85, 0x80, 0x7c, 0x78
.db 0x76, 0x76, 0x77, 0x7a, 0x7d, 0x81, 0x83, 0x85, 0x86, 0x87, 0x85, 0x84, 0x82, 0x7f, 0x7e, 0x7c
.db 0x7c, 0x7c, 0x7c, 0x7d, 0x7d, 0x7f, 0x80, 0x82, 0x84, 0x86, 0x86, 0x85, 0x82, 0x7e, 0x79, 0x77
.db 0x76, 0x78, 0x7c, 0x80, 0x84, 0x87, 0x87, 0x86, 0x83, 0x80, 0x7d, 0x7a, 0x79, 0x7a, 0x7c, 0x80
.db 0x83, 0x85, 0x84, 0x83, 0x81, 0x7f, 0x7d, 0x7c, 0x7d, 0x7d, 0x7d, 0x7e, 0x7e, 0x7f, 0x7f, 0x80
.db 0x81, 0x81, 0x82, 0x81, 0x81, 0x80, 0x7f, 0x7e, 0x7e, 0x7d, 0x7e, 0x7e, 0x7f, 0x80, 0x81, 0x81
.db 0x81, 0x81, 0x81, 0x81, 0x80, 0x80, 0x7f, 0x7e, 0x7d, 0x7c, 0x7c, 0x7c, 0x7d, 0x7e, 0x80, 0x81
.db 0x82, 0x82, 0x81, 0x80, 0x7e, 0x7c, 0x7c, 0x7d, 0x7f, 0x81, 0x83, 0x83, 0x82, 0x80, 0x7e, 0x7c
.db 0x7c, 0x7d, 0x7e, 0x80, 0x83, 0x84, 0x85, 0x84, 0x82, 0x80, 0x7e, 0x7c, 0x7a, 0x7b, 0x7c, 0x7f
.db 0x81, 0x83, 0x84, 0x83, 0x81, 0x7f, 0x7d, 0x7b, 0x7b, 0x7b, 0x7d, 0x7f, 0x82, 0x84, 0x85, 0x85
.db 0x84, 0x82, 0x80, 0x7e, 0x7d, 0x7d, 0x7e, 0x7f, 0x80, 0x81, 0x82, 0x82, 0x81, 0x80, 0x7e, 0x7d
.db 0x7c, 0x7c, 0x7d, 0x7e, 0x7f, 0x80, 0x81, 0x82, 0x83, 0x82, 0x81, 0x80, 0x80, 0x7f, 0x7f, 0x7f
.db 0x7f, 0x80, 0x80, 0x80, 0x80, 0x80, 0x7f, 0x7f, 0x7e, 0x7f, 0x7f, 0x80, 0x80, 0x81, 0x81, 0x81
.db 0x81, 0x81, 0x80, 0x80, 0x7f, 0x7e, 0x7e, 0x7d, 0x7c, 0x7b, 0x7c, 0x7d, 0x7e, 0x80, 0x82, 0x84
.db 0x85, 0x86, 0x85, 0x84, 0x82, 0x80, 0x7e, 0x7d, 0x7c, 0x7d, 0x7d, 0x7e, 0x7e, 0x7e, 0x7d, 0x7d
.db 0x7c, 0x7b, 0x7b, 0x7b, 0x7c, 0x7d, 0x7f, 0x80, 0x83, 0x84, 0x86, 0x87, 0x86, 0x85, 0x84, 0x82
.db 0x81, 0x80, 0x7f, 0x7e, 0x7e, 0x7e, 0x7d, 0x7c, 0x7a, 0x78, 0x75, 0x73, 0x71, 0x71, 0x73, 0x76
.db 0x79, 0x7e, 0x82, 0x87, 0x8a, 0x8b, 0x8b, 0x8a, 0x8a, 0x89, 0x89, 0x88, 0x88, 0x87, 0x85, 0x81
.db 0x7d, 0x78, 0x73, 0x6e, 0x6b, 0x68, 0x67, 0x67, 0x67, 0x69, 0x6b, 0x6e, 0x73, 0x7a, 0x81, 0x88
.db 0x8f, 0x95, 0x9a, 0x9e, 0xa0, 0xa1, 0xa1, 0xa0, 0x9e, 0x9c, 0x9b, 0x9a, 0x98, 0x97, 0x95, 0x91
.db 0x8b, 0x82, 0x77, 0x6a, 0x5d, 0x51, 0x49, 0x45, 0x45, 0x4a, 0x53, 0x5e, 0x6a, 0x75, 0x7f, 0x87
.db 0x8c, 0x8e, 0x8e, 0x8b, 0x88, 0x84, 0x81, 0x7f, 0x7e, 0x81, 0x85, 0x8c, 0x94, 0x9d, 0xa6, 0xae
.db 0xb3, 0xb6, 0xb5, 0xb1, 0xa9, 0x9e, 0x90, 0x80, 0x6f, 0x60, 0x52, 0x46, 0x3e, 0x3a, 0x3a, 0x3e
.db 0x46, 0x51, 0x5f, 0x6e, 0x7e, 0x8c, 0x98, 0xa1, 0xa5, 0xa3, 0x9e, 0x94, 0x88, 0x7d, 0x73, 0x6e
.db 0x6e, 0x74, 0x80, 0x90, 0xa1, 0xb0, 0xbc, 0xc4, 0xc3, 0xbc, 0xb0, 0xa0, 0x8f, 0x7e, 0x6d, 0x60
.db 0x55, 0x4d, 0x48, 0x45, 0x44, 0x45, 0x48, 0x4f, 0x58, 0x63, 0x70, 0x7e, 0x8b, 0x96, 0x9d, 0xa0
.db 0x9d, 0x97, 0x8d, 0x81, 0x76, 0x6d, 0x69, 0x6b, 0x73, 0x80, 0x91, 0xa3, 0xb4, 0xc1, 0xc8, 0xc8
.db 0xc2, 0xb6, 0xa7, 0x96, 0x84, 0x72, 0x63, 0x55, 0x4b, 0x42, 0x3d, 0x3b, 0x3c, 0x41, 0x4a, 0x57
.db 0x65, 0x76, 0x86, 0x94, 0x9f, 0xa5, 0xa5, 0xa0, 0x95, 0x88, 0x7a, 0x6d, 0x64, 0x62, 0x66, 0x72
.db 0x82, 0x96, 0xab, 0xbd, 0xcb, 0xd1, 0xd0, 0xc7, 0xb7, 0xa5, 0x90, 0x7c, 0x68, 0x58, 0x4a, 0x40
.db 0x3a, 0x37, 0x38, 0x3b, 0x43, 0x4e, 0x5c, 0x6b, 0x7b, 0x8b, 0x98, 0xa2, 0xa6, 0xa4, 0x9c, 0x90
.db 0x81, 0x72, 0x66, 0x5f, 0x60, 0x68, 0x78, 0x8d, 0xa5, 0xbb, 0xce, 0xd8, 0xdb, 0xd5, 0xc7, 0xb2
.db 0x9b, 0x83, 0x6d, 0x5a, 0x4b, 0x40, 0x3a, 0x38, 0x38, 0x3b, 0x41, 0x49, 0x53, 0x5f, 0x6e, 0x7d
.db 0x8c, 0x9a, 0xa3, 0xa6, 0xa4, 0x9c, 0x8f, 0x81, 0x74, 0x6a, 0x67, 0x6b, 0x76, 0x87, 0x9c, 0xb1
.db 0xc3, 0xcf, 0xd3, 0xcf, 0xc4, 0xb2, 0x9e, 0x87, 0x72, 0x5e, 0x4e, 0x42, 0x3c, 0x39, 0x3a, 0x3d
.db 0x42, 0x49, 0x52, 0x5e, 0x6c, 0x7b, 0x8b, 0x9a, 0xa4, 0xa9, 0xa7, 0xa0, 0x92, 0x82, 0x73, 0x67
.db 0x61, 0x63, 0x6d, 0x7e, 0x93, 0xaa, 0xbe, 0xcd, 0xd4, 0xd2, 0xc8, 0xb7, 0xa2, 0x8b, 0x75, 0x62
.db 0x52, 0x47, 0x41, 0x3d, 0x3c, 0x3d, 0x40, 0x45, 0x4d, 0x58, 0x67, 0x77, 0x89, 0x9a, 0xa8, 0xaf
.db 0xae, 0xa7, 0x99, 0x87, 0x76, 0x68, 0x61, 0x62, 0x6c, 0x7d, 0x92, 0xa8, 0xbc, 0xca, 0xd0, 0xce
.db 0xc4, 0xb2, 0x9e, 0x88, 0x74, 0x62, 0x54, 0x4a, 0x43, 0x3f, 0x3d, 0x3d, 0x3f, 0x44, 0x4d, 0x5a
.db 0x6a, 0x7d, 0x8f, 0x9f, 0xaa, 0xad, 0xa9, 0x9e, 0x8e, 0x7d, 0x6e, 0x66, 0x66, 0x6e, 0x7e, 0x92
.db 0xa7, 0xba, 0xc8, 0xcc, 0xc9, 0xbe, 0xae, 0x9c, 0x89, 0x77, 0x68, 0x5b, 0x51, 0x48, 0x40, 0x3a
.db 0x36, 0x36, 0x3a, 0x43, 0x51, 0x64, 0x7b, 0x91, 0xa5, 0xb3, 0xb9, 0xb6, 0xaa, 0x99, 0x87, 0x77
.db 0x6e, 0x6e, 0x75, 0x83, 0x94, 0xa6, 0xb4, 0xbc, 0xbc, 0xb7, 0xad, 0xa0, 0x92, 0x85, 0x7a, 0x6f
.db 0x65, 0x5a, 0x4f, 0x42, 0x36, 0x2d, 0x29, 0x2c, 0x38, 0x4b, 0x64, 0x80, 0x9a, 0xaf, 0xbc, 0xbf
.db 0xb9, 0xad, 0x9e, 0x8f, 0x84, 0x80, 0x83, 0x8d, 0x9b, 0xa8, 0xb2, 0xb5, 0xb0, 0xa6, 0x97, 0x88
.db 0x7a, 0x6f, 0x66, 0x60, 0x59, 0x52, 0x49, 0x40, 0x39, 0x37, 0x3d, 0x4a, 0x5e, 0x75, 0x8c, 0x9f
.db 0xac, 0xaf, 0xaa, 0x9f, 0x90, 0x84, 0x7e, 0x7f, 0x89, 0x99, 0xac, 0xbc, 0xc5, 0xc4, 0xb9, 0xa6
.db 0x91, 0x7c, 0x6c, 0x62, 0x5e, 0x5d, 0x5c, 0x5a, 0x55, 0x4f, 0x49, 0x46, 0x49, 0x52, 0x61, 0x74
.db 0x87, 0x94, 0x9b, 0x99, 0x90, 0x85, 0x7b, 0x76, 0x7b, 0x89, 0x9f, 0xb7, 0xcc, 0xd7, 0xd7, 0xcb
.db 0xb6, 0x9d, 0x84, 0x6f, 0x60, 0x57, 0x51, 0x4e, 0x49, 0x43, 0x3d, 0x39, 0x3b, 0x44, 0x55, 0x6c
.db 0x85, 0x99, 0xa5, 0xa6, 0x9d, 0x8e, 0x7f, 0x75, 0x74, 0x7e, 0x90, 0xa6, 0xbb, 0xcc, 0xd2, 0xcf
.db 0xc3, 0xb0, 0x9c, 0x8a, 0x7a, 0x6e, 0x63, 0x57, 0x49, 0x39, 0x2b, 0x21, 0x21, 0x2b, 0x40, 0x5c
.db 0x7a, 0x93, 0xa3, 0xa8, 0xa2, 0x97, 0x8b, 0x83, 0x83, 0x8c, 0x9b, 0xac, 0xbc, 0xc7, 0xca, 0xc5
.db 0xba, 0xac, 0x9d, 0x8f, 0x83, 0x77, 0x69, 0x59, 0x45, 0x2f, 0x1d, 0x14, 0x16, 0x24, 0x3c, 0x58
.db 0x74, 0x8b, 0x99, 0x9f, 0x9e, 0x99, 0x95, 0x95, 0x9b, 0xa6, 0xb5, 0xc3, 0xca, 0xca, 0xc2, 0xb5
.db 0xa9, 0xa0, 0x99, 0x8f, 0x80, 0x6b, 0x52, 0x3a, 0x27, 0x1d, 0x1d, 0x25, 0x33, 0x44, 0x58, 0x6d
.db 0x80, 0x8f, 0x98, 0x9c, 0x9b, 0x99, 0x9a, 0x9f, 0xa8, 0xb1, 0xb9, 0xbc, 0xbd, 0xbd, 0xbc, 0xb8
.db 0xb0, 0xa2, 0x8d, 0x74, 0x5b, 0x41, 0x2b, 0x1a, 0xf, 0xe, 0x16, 0x25, 0x39, 0x50, 0x6b, 0x87
.db 0xa1, 0xb5, 0xbf, 0xc1, 0xbe, 0xbd, 0xbe, 0xc2, 0xbf, 0xb7, 0xae, 0xa5, 0xa1, 0x9f, 0x9c, 0x91
.db 0x7e, 0x67, 0x50, 0x3e, 0x33, 0x2d, 0x28, 0x23, 0x23, 0x29, 0x3a, 0x54, 0x73, 0x91, 0xa6, 0xb4
.db 0xbd, 0xc7, 0xd0, 0xd4, 0xd0, 0xc6, 0xb7, 0xad, 0xa7, 0xa4, 0x9f, 0x94, 0x81, 0x69, 0x53, 0x43
.db 0x38, 0x2f, 0x27, 0x21, 0x22, 0x2f, 0x40, 0x51, 0x68, 0x81, 0x9b, 0xb5, 0xc7, 0xd1, 0xd8, 0xd9
.db 0xd6, 0xcb, 0xb8, 0xa7, 0x9c, 0x96, 0x93, 0x8a, 0x7a, 0x69, 0x59, 0x4c, 0x41, 0x37, 0x2f, 0x2b
.db 0x2b, 0x31, 0x3c, 0x4c, 0x63, 0x7a, 0x91, 0xa7, 0xbb, 0xd1, 0xe4, 0xec, 0xea, 0xe0, 0xd1, 0xc5
.db 0xb8, 0xa5, 0x91, 0x7a, 0x63, 0x54, 0x44, 0x32, 0x22, 0x19, 0x17, 0x1d, 0x21, 0x28, 0x39, 0x51
.db 0x6f, 0x8c, 0xa2, 0xb6, 0xcc, 0xe0, 0xf0, 0xf3, 0xeb, 0xe0, 0xd1, 0xc2, 0xae, 0x95, 0x7a, 0x63
.db 0x51, 0x3d, 0x2b, 0x1f, 0x1d, 0x20, 0x22, 0x27, 0x37, 0x4b, 0x60, 0x77, 0x8f, 0xa7, 0xba, 0xcb
.db 0xdd, 0xed, 0xf1, 0xea, 0xe0, 0xd5, 0xc3, 0xa6, 0x88, 0x70, 0x5d, 0x46, 0x30, 0x24, 0x1e, 0x1c
.db 0x1c, 0x1d, 0x2c, 0x3f, 0x4e, 0x66, 0x81, 0x97, 0xaa, 0xba, 0xce, 0xe1, 0xeb, 0xed, 0xe7, 0xe3
.db 0xd7, 0xbe, 0xa2, 0x88, 0x6e, 0x51, 0x33, 0x1f, 0x15, 0xf, 0xa, 0xf, 0x1e, 0x32, 0x48, 0x61
.db 0x82, 0x9c, 0xaf, 0xc8, 0xda, 0xe4, 0xec, 0xef, 0xeb, 0xdd, 0xd2, 0xc1, 0xa0, 0x8d, 0x79, 0x5b
.db 0x44, 0x31, 0x20, 0x1a, 0x10, 0xc, 0x1b, 0x25, 0x34, 0x54, 0x6e, 0x85, 0xa3, 0xba, 0xcf, 0xe2
.db 0xec, 0xf6, 0xf6, 0xea, 0xe2, 0xce, 0xad, 0x99, 0x81, 0x5c, 0x47, 0x31, 0x1c, 0x17, 0xc, 0x6
.db 0x16, 0x1c, 0x2d, 0x51, 0x65, 0x7d, 0xa0, 0xb4, 0xca, 0xe1, 0xe8, 0xf4, 0xfa, 0xec, 0xe5, 0xd2
.db 0xb1, 0xa3, 0x87, 0x62, 0x51, 0x37, 0x1e, 0x18, 0xc, 0x7, 0x13, 0x19, 0x2b, 0x4a, 0x5a, 0x75
.db 0x98, 0xa8, 0xc4, 0xdb, 0xde, 0xf1, 0xf7, 0xe9, 0xe8, 0xda, 0xbb, 0xaa, 0x91, 0x70, 0x57, 0x41
.db 0x2e, 0x19, 0xf, 0xe, 0xa, 0x13, 0x2b, 0x3c, 0x4e, 0x72, 0x8c, 0x9f, 0xc3, 0xd5, 0xdb, 0xf2
.db 0xf4, 0xe5, 0xe7, 0xd7, 0xba, 0xaf, 0x96, 0x74, 0x65, 0x4e, 0x34, 0x28, 0x1c, 0x13, 0x13, 0x1a
.db 0x29, 0x3a, 0x4c, 0x6a, 0x81, 0x97, 0xb8, 0xc7, 0xd4, 0xee, 0xeb, 0xe6, 0xec, 0xd5, 0xbf, 0xb5
.db 0x92, 0x78, 0x69, 0x49, 0x36, 0x2a, 0x18, 0x14, 0x17, 0x19, 0x2b, 0x3c, 0x4b, 0x69, 0x7e, 0x94
.db 0xb5, 0xc4, 0xd3, 0xe9, 0xe7, 0xe6, 0xea, 0xd6, 0xc8, 0xbb, 0x9b, 0x84, 0x73, 0x54, 0x41, 0x35
.db 0x21, 0x1c, 0x1c, 0x19, 0x26, 0x36, 0x40, 0x5e, 0x72, 0x84, 0xa4, 0xb4, 0xc5, 0xd9, 0xe2, 0xe1
.db 0xe6, 0xde, 0xcc, 0xc5, 0xaa, 0x93, 0x87, 0x64, 0x53, 0x40, 0x2b, 0x27, 0x1a, 0x1e, 0x1e, 0x2c
.db 0x3c, 0x4a, 0x67, 0x78, 0x90, 0xaa, 0xb9, 0xcb, 0xdb, 0xdc, 0xe1, 0xe0, 0xd5, 0xca, 0xba, 0xa3
.db 0x8f, 0x7f, 0x60, 0x52, 0x40, 0x2b, 0x29, 0x1f, 0x1c, 0x27, 0x2f, 0x3b, 0x52, 0x63, 0x78, 0x91
.db 0xa3, 0xb6, 0xc7, 0xd1, 0xd8, 0xdb, 0xda, 0xd3, 0xc8, 0xbc, 0xa8, 0x97, 0x85, 0x6e, 0x5d, 0x4c
.db 0x3a, 0x2f, 0x28, 0x23, 0x26, 0x2e, 0x36, 0x47, 0x59, 0x6b, 0x83, 0x95, 0xa8, 0xba, 0xc5, 0xd0
.db 0xd4, 0xd4, 0xd1, 0xca, 0xbe, 0xb3, 0xa6, 0x92, 0x85, 0x71, 0x61, 0x51, 0x42, 0x37, 0x2f, 0x2d
.db 0x2a, 0x32, 0x37, 0x45, 0x54, 0x64, 0x78, 0x8a, 0x9d, 0xac, 0xbb, 0xc2, 0xca, 0xcc, 0xca, 0xc7
.db 0xbe, 0xb5, 0xab, 0xa0, 0x92, 0x86, 0x79, 0x6c, 0x5f, 0x54, 0x4a, 0x40, 0x3c, 0x38, 0x39, 0x3d
.db 0x44, 0x4c, 0x57, 0x66, 0x72, 0x82, 0x8e, 0x9c, 0xa6, 0xae, 0xb8, 0xbb, 0xc1, 0xbf, 0xbe, 0xba
.db 0xb3, 0xab, 0x9f, 0x96, 0x88, 0x7c, 0x6e, 0x63, 0x58, 0x4e, 0x48, 0x43, 0x42, 0x42, 0x46, 0x4b
.db 0x53, 0x5b, 0x65, 0x6f, 0x79, 0x83, 0x8b, 0x97, 0x9d, 0xa7, 0xaf, 0xb3, 0xb7, 0xba, 0xb8, 0xb5
.db 0xb2, 0xa7, 0x9f, 0x95, 0x87, 0x7b, 0x70, 0x63, 0x5a, 0x54, 0x4c, 0x4c, 0x4a, 0x4a, 0x4e, 0x53
.db 0x57, 0x5e, 0x65, 0x6b, 0x76, 0x7c, 0x86, 0x90, 0x99, 0xa1, 0xaa, 0xaf, 0xb4, 0xb8, 0xb7, 0xb4
.db 0xb0, 0xa7, 0x9d, 0x94, 0x86, 0x7c, 0x70, 0x66, 0x5d, 0x58, 0x52, 0x51, 0x50, 0x51, 0x54, 0x58
.db 0x5d, 0x62, 0x69, 0x6e, 0x76, 0x7d, 0x85, 0x8c, 0x94, 0x9a, 0xa1, 0xa6, 0xa9, 0xac, 0xac, 0xaa
.db 0xa9, 0xa3, 0x9e, 0x96, 0x8d, 0x84, 0x7b, 0x72, 0x69, 0x63, 0x5c, 0x59, 0x56, 0x56, 0x56, 0x59
.db 0x5c, 0x63, 0x67, 0x6f, 0x75, 0x7c, 0x84, 0x8a, 0x91, 0x96, 0x9b, 0x9d, 0xa0, 0x9f, 0xa0, 0x9e
.db 0x9c, 0x99, 0x95, 0x92, 0x8b, 0x87, 0x80, 0x7c, 0x76, 0x70, 0x6d, 0x68, 0x67, 0x64, 0x66, 0x66
.db 0x6b, 0x6c, 0x70, 0x76, 0x77, 0x7e, 0x80, 0x84, 0x88, 0x89, 0x8b, 0x8d, 0x8d, 0x8e, 0x8c, 0x8d
.db 0x8a, 0x8b, 0x87, 0x88, 0x82, 0x83, 0x80, 0x7f, 0x7d, 0x7c, 0x7b, 0x79, 0x7b, 0x78, 0x79, 0x77
.db 0x79, 0x76, 0x7b, 0x76, 0x7c, 0x78, 0x7e, 0x7c, 0x80, 0x80, 0x82, 0x85, 0x83, 0x89, 0x85, 0x8b
.db 0x85, 0x8d, 0x85, 0x8c, 0x87, 0x87, 0x87, 0x83, 0x84, 0x7f, 0x82, 0x7b, 0x7f, 0x78, 0x79, 0x76
.db 0x75, 0x73, 0x73, 0x72, 0x72, 0x74, 0x74, 0x77, 0x7a, 0x7c, 0x80, 0x85, 0x86, 0x8d, 0x8d, 0x90
.db 0x90, 0x91, 0x90, 0x90, 0x90, 0x8b, 0x8c, 0x87, 0x86, 0x82, 0x82, 0x7d, 0x7d, 0x78, 0x78, 0x77
.db 0x74, 0x78, 0x74, 0x77, 0x76, 0x78, 0x78, 0x7b, 0x7c, 0x7d, 0x80, 0x81, 0x81, 0x84, 0x81, 0x85
.db 0x81, 0x86, 0x81, 0x85, 0x82, 0x82, 0x83, 0x7e, 0x82, 0x7c, 0x81, 0x7a, 0x80, 0x7a, 0x80, 0x7c
.db 0x7e, 0x7e, 0x7f, 0x7f, 0x7f, 0x80, 0x7f, 0x84, 0x7f, 0x86, 0x82, 0x86, 0x82, 0x85, 0x82, 0x83
.db 0x80, 0x80, 0x80, 0x7e, 0x7e, 0x7c, 0x7d, 0x79, 0x7d, 0x78, 0x7b, 0x7a, 0x7c, 0x7b, 0x7e, 0x7d
.db 0x7f, 0x82, 0x80, 0x84, 0x81, 0x85, 0x80, 0x85, 0x80, 0x82, 0x80, 0x7f, 0x80, 0x7d, 0x7f, 0x7b
.db 0x7e, 0x7a, 0x7e, 0x7c, 0x7e, 0x7e, 0x7e, 0x7f, 0x7e, 0x81, 0x7f, 0x83, 0x7f, 0x83, 0x82, 0x82
.db 0x85, 0x82, 0x84, 0x82, 0x83, 0x81, 0x81, 0x7f, 0x7d, 0x7e, 0x7b, 0x7b, 0x7b, 0x7a, 0x7b, 0x7a
.db 0x7d, 0x7b, 0x7f, 0x7f, 0x7f, 0x83, 0x82, 0x84, 0x83, 0x86, 0x83, 0x86, 0x85, 0x82, 0x85, 0x80
.db 0x82, 0x7f, 0x80, 0x7e, 0x7d, 0x7f, 0x7b, 0x7f, 0x7a, 0x7c, 0x7b, 0x7a, 0x7a, 0x7a, 0x7c, 0x7a
.db 0x7f, 0x7c, 0x7f, 0x80, 0x82, 0x82, 0x82, 0x85, 0x82, 0x87, 0x82, 0x88, 0x83, 0x84, 0x85, 0x80
.db 0x83, 0x7f, 0x81, 0x7c, 0x81, 0x7b, 0x7c, 0x7b, 0x7b, 0x7a, 0x7b, 0x7c, 0x7a, 0x7f, 0x7d, 0x7f
.db 0x81, 0x81, 0x81, 0x83, 0x83, 0x83, 0x82, 0x84, 0x7f, 0x85, 0x80, 0x80, 0x82, 0x7d, 0x81, 0x7e
.db 0x7f, 0x7c, 0x7f, 0x7c, 0x7e, 0x7e, 0x7e, 0x7d, 0x80, 0x7f, 0x7d, 0x84, 0x7b, 0x83, 0x7e, 0x81
.db 0x81, 0x81, 0x83, 0x7f, 0x85, 0x7f, 0x83, 0x80, 0x80, 0x81, 0x80, 0x80, 0x7f, 0x7f, 0x7d, 0x7e
.db 0x7e, 0x7c, 0x7e, 0x7d, 0x7c, 0x7e, 0x7e, 0x7d, 0x80, 0x81, 0x7d, 0x84, 0x7f, 0x81, 0x84, 0x81
.db 0x83, 0x81, 0x83, 0x80, 0x82, 0x80, 0x80, 0x80, 0x7f, 0x7c, 0x80, 0x7a, 0x7d, 0x7f, 0x7a, 0x7e
.db 0x7c, 0x7d, 0x7c, 0x80, 0x7c, 0x7f, 0x7f, 0x7f, 0x80, 0x81, 0x80, 0x81, 0x83, 0x80, 0x83, 0x82
.db 0x82, 0x81, 0x83, 0x80, 0x81, 0x82, 0x80, 0x80, 0x82, 0x7f, 0x7f, 0x7f, 0x7e, 0x7e, 0x7c, 0x7e
.db 0x7c, 0x7b, 0x7e, 0x7c, 0x7d, 0x80, 0x7d, 0x81, 0x80, 0x81, 0x82, 0x82, 0x83, 0x82, 0x84, 0x83
.db 0x81, 0x85, 0x7f, 0x82, 0x82, 0x7e, 0x81, 0x7f, 0x7e, 0x7e, 0x7d, 0x7c, 0x7d, 0x7a, 0x7c, 0x7b
.db 0x7c, 0x7c, 0x7d, 0x7e, 0x7e, 0x80, 0x81, 0x80, 0x82, 0x83, 0x82, 0x83, 0x84, 0x83, 0x83, 0x81
.db 0x83, 0x80, 0x7f, 0x81, 0x7d, 0x7e, 0x7e, 0x7d, 0x7c, 0x7c, 0x7e, 0x7c, 0x7c, 0x7e, 0x7e, 0x7e
.db 0x7d, 0x7f, 0x80, 0x7f, 0x81, 0x83, 0x81, 0x82, 0x83, 0x81, 0x82, 0x82, 0x80, 0x82, 0x80, 0x7f
.db 0x81, 0x7e, 0x7f, 0x7e, 0x80, 0x7e, 0x7f, 0x80, 0x7d, 0x7e, 0x7f, 0x7c, 0x7e, 0x7e, 0x7d, 0x7e
.db 0x7f, 0x80, 0x7f, 0x81, 0x83, 0x81, 0x83, 0x83, 0x81, 0x82, 0x81, 0x7f, 0x80, 0x80, 0x7e, 0x7e
.db 0x7f, 0x7e, 0x7e, 0x7e, 0x7f, 0x7f, 0x7f, 0x80, 0x81, 0x7f, 0x80, 0x82, 0x80, 0x80, 0x80, 0x80
.db 0x80, 0x80, 0x7e, 0x7e, 0x7e, 0x7e, 0x7e, 0x7e, 0x7e, 0x7f, 0x80, 0x7f, 0x80, 0x81, 0x82, 0x82
.db 0x81, 0x81, 0x82, 0x81, 0x7f, 0x80, 0x80, 0x7e, 0x7d, 0x7f, 0x7e, 0x7d, 0x7e, 0x7e, 0x7f, 0x7f
.db 0x7e, 0x7f, 0x7f, 0x80, 0x7e, 0x7f, 0x81, 0x7f, 0x7f, 0x80, 0x81, 0x81, 0x82, 0x83, 0x83, 0x82
.db 0x82, 0x82, 0x82, 0x81, 0x81, 0x7f, 0x7e, 0x7e, 0x7d, 0x7b, 0x7c, 0x7c, 0x7d, 0x7d, 0x7e, 0x7f
.db 0x7e, 0x7f, 0x80, 0x7f, 0x80, 0x82, 0x81, 0x81, 0x83, 0x83, 0x82, 0x82, 0x82, 0x81, 0x81, 0x81
.db 0x80, 0x7f, 0x7f, 0x7e, 0x7e, 0x7f, 0x7d, 0x7d, 0x7f, 0x7d, 0x7d, 0x7e, 0x7f, 0x7e, 0x7e, 0x7f
.db 0x7f, 0x7f, 0x80, 0x81, 0x81, 0x81, 0x82, 0x83, 0x81, 0x81, 0x81, 0x81, 0x80, 0x80, 0x80, 0x80
.db 0x7e, 0x7f, 0x7f, 0x7e, 0x7e, 0x7e, 0x7e, 0x7f, 0x7e, 0x7e, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x80
.db 0x80, 0x7f, 0x7f, 0x80, 0x80, 0x7f, 0x80, 0x80, 0x80, 0x7f, 0x7f, 0x80, 0x80, 0x7f, 0x7f, 0x80
.db 0x80, 0x7f, 0x7f, 0x80, 0x80, 0x7f, 0x7f, 0x80, 0x80, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f
.db 0x7f, 0x80, 0x80, 0x80, 0x80, 0x81, 0x80, 0x80, 0x81, 0x80, 0x7f, 0x7f, 0x80, 0x80, 0x7f, 0x7f
.db 0x80, 0x80, 0x7f, 0x7f, 0x80, 0x81, 0x7f, 0x7f, 0x80, 0x80, 0x7f, 0x7e, 0x80, 0x80, 0x7f, 0x7f
.db 0x80, 0x80, 0x7f, 0x7f, 0x80, 0x81, 0x80, 0x7f, 0x80, 0x80, 0x7f, 0x7f, 0x80, 0x80, 0x7f, 0x7f
.db 0x7f, 0x7f, 0x7f, 0x7f, 0x80, 0x7f, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x7f
.db 0x7f, 0x7f, 0x7e, 0x7e, 0x7e, 0x7f, 0x7f, 0x7f, 0x80, 0x80, 0x80, 0x7f, 0x7b, 0x77, 0x73, 0x6f
.db 0x6b, 0x67, 0x63, 0x5f, 0x5b, 0x57, 0x53, 0x4f, 0x4b, 0x47, 0x43, 0x3f, 0x3b, 0x37, 0x33, 0x2f
.db 0x2b, 0x27, 0x23, 0x1f, 0x1b, 0x17, 0x13, 0xf, 0xb, 0x7, 0x3, 0x0, 0x0
