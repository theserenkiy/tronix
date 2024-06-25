;chip t2313
.include "C:/avr/includes/tn2313def.inc"

.equ FCLK = 12000000

;ON PORTB:
.equ RS = 5
.equ E = 4
;PB3...PB0 => D7...D4 of LCD


RESET:
	; sbi DDRD, LED0
	; sbi DDRD, LED1
	; cbi PORTD, LED0
	; cbi PORTD, LED1
	ldi r16,0xff
	out DDRB, r16
	ldi r16,(1 << E) | (1 << RS)
	out PORTB,r16

MAIN:
	rcall LCD_INIT

	;ldi r16,0b10000001
	;rcall LCD_CMD

	; ldi r16, 2
	; rcall DELAY_MS

	ldi r20,4
_loop:
	ldi r16,0b01101000
	rcall LCD_WRITE
	ldi r16,100
	rcall DELAY_US
	dec r20
	brne _loop


	; ldi r16, 2
	; rcall DELAY_MS

	rjmp PC
	rjmp MAIN


;#########################################
LCD_INIT:
	ldi r16,100
	rcall DELAY_MS

	ldi r16,0b00000011
	rcall LCD_CMD_4B

	ldi r16, 1
	rcall DELAY_MS

	ldi r16,0b00100100
	rcall LCD_CMD
	ldi r16,0b00100100
	rcall LCD_CMD
	ldi r16,0b00001111
	rcall LCD_CMD
	ldi r16,0b00000001
	rcall LCD_CMD

	ldi r16, 4
	rcall DELAY_MS

	ldi r16,0b00000111
	rcall LCD_CMD

	ret


;#########################################
;r16 - cmd
LCD_CMD_4B:
	cbi PORTB, RS
	sbi PORTB, E
	ori r16, (1 << E)
	out PORTB, r16
	nop
	nop
	cbi PORTB, E
	nop 
	nop
	sbi PORTB, RS
	sbi PORTB, E
	ret

;#########################################
;r16 - cmd
LCD_CMD:
	cbi PORTB, RS
	rcall LCD_WRITE
	sbi PORTB, RS
	ldi r16, 50
	rcall DELAY_US
	ret

;#########################################
;r16 - data
LCD_WRITE:
	nop
	nop
	sbi PORTB, E  
	mov r17,r16
	lsr r17
	lsr r17
	lsr r17
	lsr r17
	ori r17, (1 << E)
	out PORTB, r17
	nop
	nop
	nop
	cbi PORTB, E
	nop
	nop
	nop 
	sbi PORTB, E
	andi r16,0x0f
	ori r16, (1 << E)
	out PORTB, r16
	nop
	nop
	nop
	cbi PORTB, E
	nop
	nop
	nop
	;sbi PORTB, E
	ret


;#########################################
;r16 - us to wait
DELAY_US:
_loop:
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	dec r16
	brne _loop
	ret	


;#########################################
;r16 - ms to wait
DELAY_MS:
	ldi r17,0
_loop:
	nop
	dec r17
	brne _loop
	dec r16
	brne _loop
	ret	

;#########################################
DELAY_1S:
	ldi r18,0
	ldi r17,0
	ldi r16,61
_loop:
	dec r18
	brne _loop
	dec r17
	brne _loop
	dec r16
	brne _loop
	ret