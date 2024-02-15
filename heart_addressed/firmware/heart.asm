;chip t13
.include "c:/avr/includes/tn13def.inc"

.equ LED_PIN = 4
.equ LEDEN_PIN = 0
.equ MEASEN_PIN = 1
.equ MEASURE_PIN = 2

.def R = r23
.def G = r24
.def B = r25

.dseg
image: 	.byte 48

.cseg
.org 0
RESET: 
	sbi DDRB,LED_PIN
	sbi DDRB,LEDEN_PIN
	sbi PORTB, LEDEN_PIN

	ldi r22,0
MAIN:
	;rcall LEDS_WRITE
	;rcall DUMMY_IMAGE
	mov r16,r22
	inc r22
	rcall RAINBOW
	rcall WRITE_LEDS

	rcall DELAY
	rjmp MAIN

;##########################################################
;writes the array `image` to LEDs
WRITE_LEDS:
	ldi r16, 48
	ldi YL, low(image)
	ldi YH, high(image)
	
_dump_byte:
	ld r17,Y+
	ldi r18,8
_dump_bit:
	rol r17
	brcs _one
	sbi PORTB, LED_PIN
	nop
	nop
	nop
	cbi PORTB, LED_PIN
	nop
	nop
	nop
	nop
	nop
	rjmp _end_bit
_one:
	sbi PORTB, LED_PIN
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	cbi PORTB, LED_PIN
_end_bit:
	dec r18
	brne _dump_bit
	dec r16
	brne _dump_byte
	ret
;end WRITE_LEDS
;##########################################################


;##########################################################
;in:	r16 - hue, 0..31
;out:	r20,21,22 - RGB
HUE2COLOR:
	ldi R,0
	ldi G,0
	ldi B,0
	andi r16,0b00011111
	cpi r16,5
	brcc _10
	ldi B, 5
	mov R,r16
	rjmp _end
_10:
	cpi r16,10
	brcc _15
	ldi B,10
	sub B,r16
	ldi R,5
	rjmp _end
_15:
	cpi r16,15
	brcc _20
	ldi R,5
	mov G,r16
	subi G,10
	rjmp _end
_20:
	cpi r16,20
	brcc _25
	ldi G,5
	ldi R,20
	sub R,r16
	rjmp _end
_25:
	cpi r16,25
	brcc _30
	ldi G,5
	mov B,r16
	subi B,20
	rjmp _end
_30:
	cpi r16,30
	brcc _32
	ldi B,5
	ldi G,30
	sub G,r16
	rjmp _end
_32:
	ldi B,5
	ldi R,1
_end:
	lsl R
	;lsl R
	lsl G
	;lsl G
	lsl B
	;lsl B
	ret


;######################################################
;Places R G B registers to array `image` at pointer Y
RGB2MEM:
	st Y+,G
	st Y+,R
	st Y+,B
	ret

;######################################################
;Draws rainbow starting at hue in r16
RAINBOW:
	ldi YL, low(image)
	ldi YH, high(image)
	mov r20,r16
	ldi r21,16
_FILL:
	mov r16,r20
	rcall HUE2COLOR
	rcall RGB2MEM
	inc r20
	inc r20
	dec r21
	brne _FILL
	ret	


DELAY_50us:
	ldi r16,0
_loop:
	dec r16
	brne _loop
	ret


DELAY:
	ldi r16,0
	ldi r17,0
_loop:
	dec r16
	brne _loop
	dec r17
	brne _loop
	ret
