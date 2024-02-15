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
;writes the array `image` to LEDs, shifting each value 4 times right
;uses: r16, r17, r18
WRITE_LEDS_SHIFTED:
	ldi r16, 48
	ldi YL, low(image)
	ldi YH, high(image)
	
_dump_byte:
	ld r17,Y+
	ldi r18,8
;first 4 bits are 0
_zero_bit:
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
	nop
	dec r18
	cpi r18,4
	brcc _zero_bit

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
;end WRITE_LEDS_SHIFTED
;##########################################################


;##########################################################
;in:	r16 - hue, 0..31
;out:	R,G,B
;uses:	r16
.equ HUEBITS = 8
.equ HUEMAX = (1 << HUEBITS) - 1
.equ HUESTEP = HUEMAX/6
HUE2COLOR:
	ldi R,0
	ldi G,0
	ldi B,0
	andi r16,HUEMAX
	cpi r16,HUESTEP
	brcc _p2
	ldi B, HUESTEP
	mov R,r16
	rjmp _end
_p2:
	cpi r16,HUESTEP*2
	brcc _p3
	ldi R,HUESTEP
    ldi B,HUESTEP*2
	sub B,r16
	rjmp _end
_p3:
	cpi r16,HUESTEP*3
	brcc _p4
	ldi R,HUESTEP
	mov G,r16
	subi G,HUESTEP*2
	rjmp _end
_p4:
	cpi r16,HUESTEP*4
	brcc _p5
	ldi G,HUESTEP
	ldi R,HUESTEP*4
	sub R,r16
	rjmp _end
_p5:
	cpi r16,HUESTEP*5
	brcc _p6
	ldi G,HUESTEP
	mov B,r16
	subi B,HUESTEP*4
	rjmp _end
_p6:
	cpi r16,HUESTEP*6
	brcc _p7
	ldi B,HUESTEP
	ldi G,HUESTEP*6
	sub G,r16
	rjmp _end
_p7:
	ldi B,HUESTEP
	ldi R,1
_end:
	lsl R
	lsl G
	lsl B
    lsl R
	lsl G
	lsl B
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
;in: r16 - starting hue
;out: none
;uses: r16, r20, r21
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

;######################################################
;Shifts RIGHT each LED in memory fot 1 bit
;in: none
;out: none
;uses: r16, r17
DIM_OUT:
	ldi YL, low(image)
	ldi YH, high(image)
	ldi r16,48
	ld r17,Y
	lsr r17
	st Y+,r17
	ret

;######################################################
;Shifts LEFT each LED in memory fot 1 bit
;in: none
;out: none
;uses: r16, r17
DIM_IN:
	ldi YL, low(image)
	ldi YH, high(image)
	ldi r16,48
	ld r17,Y
	lsl r17
	st Y+,r17
	ret

;######################################################
;Copy right heart branch (1..7 pixels) to left branch (15..9 pixels)
;in: none
;out: none
;uses: r16,R,G,B
BRANCH:
	ldi YL, low(image+3)
	ldi YH, high(image+3)
	ldi XL, low(image+45)
	ldi XH, high(image+45)
	ldi r16,7
_loop:
	ld G,Y+
	ld R,Y+
	ld B,Y+
	st X-,B
	st X-,R
	st X-,G
	dec r16
	brne _loop
	ret

;######################################################
;Writes RGB registers to the bottom pixel of right branch, 
;shifting the branch up.
;! Need to call BRANCH afterwards!
;in: RGB
BRANCH_SHIFT_UP:
	ldi YL, low(image+3)
	ldi YH, high(image+3)
	ldi XL, low(image)
	ldi XH, high(image)
	ldi r16,24
_loop:
	ld r17,Y+
	st X+,r17
	dec r16
	brne _loop
	st X+,G
	st X+,R
	st X+,B
	ret

;######################################################
;Writes RGB registers to the TOP PIXEL of right branch, 
;shifting the branch DOWN.
;! Need to call BRANCH afterwards!
;in: RGB
BRANCH_SHIFT_DOWN:
	ldi YL, low(image+24)
	ldi YH, high(image+24)
	ldi XL, low(image+27)
	ldi XH, high(image+27)
	ldi r16,24
_loop:
	ld r17,Y-
	st X-,r17
	dec r16
	brne _loop
	st X-,B
	st X-,R
	st X-,G
	ret

;######################################################
;in: R, G, B, r16 - brightness (0-15)
BRIGHTNESS:
	andi r16,0x0f
	
	ret

;######################################################
DELAY_50us:
	ldi r16,0
_loop:
	dec r16
	brne _loop
	ret

;######################################################
DELAY:
	ldi r16,0
	ldi r17,0
_loop:
	dec r16
	brne _loop
	dec r17
	brne _loop
	ret
