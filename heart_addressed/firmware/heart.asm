;chip t13
.include "c:/avr/includes/tn13def.inc"

.equ LED_PIN = 4
.equ LEDEN_PIN = 0
.equ MEASEN_PIN = 1
.equ MEASURE_PIN = 2

.equ HUEBITS = 8
.equ HUEMAX = (1 << HUEBITS) - 1
.equ HUESTEP = HUEMAX/6

.def R = r23
.def G = r24
.def B = r25

.def Ro = r13
.def Go = r14
.def Bo = r15

.macro ldil
	ldi r19,@1
	mov @0, r19
.endmacro

.dseg
image: 	.byte 48

.cseg
.org 0
RESET: 
	ldi r16,RAMEND
	out SPL,r16
	sbi DDRB,LED_PIN
	sbi DDRB,LEDEN_PIN
	sbi PORTB, LEDEN_PIN

	ldi r16,HUESTEP*4
MAIN:
_decr:
	mov r9,r16
	ldi r16,HUESTEP*4+1  
	rcall HUE2COLOR
	rcall HEARTBEAT
	mov r16,r9
	subi r16,1
	cpi r16,(HUESTEP*35)/10
	brcc _decr
_incr:
	mov r9,r16
	ldi r16,HUESTEP*4-1
	rcall HUE2COLOR
	rcall HEARTBEAT
	mov r16,r9
	subi r16,-1
	cpi r16,(HUESTEP*43)/10
	brcs _incr

	rjmp MAIN


.include "drv.asm"

FILLNSHOW:
	;rcall FILL
	;rcall WRITE_LEDS
	rcall HEARTBEAT
	ldi r16,100
	rcall DELAY
	ret 

;######################################################
;RGB - color, r16 - speed, r17 - brightness to stop at
FADEOUT:
	mov Ro,R
	mov Go,G
	mov Bo,B
	mov r11,r16		;speed
	mov r10,r17		;stop at
	ldil r12,63		;brightness
_loop:
	mov r16,r12
	dec r12
	rcall LED_BRIGHTNESS
	rcall FILL
	rcall WRITE_LEDS
	mov r16,r11
	rcall DELAY
	mov R,Ro
	mov G,Go
	mov B,Bo
	cp r12,r10
	brne _loop
	ret

;######################################################
HEARTBEAT:
	ldi r16,5
	ldi r17,10
	rcall FADEOUT
	ldi r16,10
	ldi r17,4
	rcall FADEOUT
	ldi r16,80
	rcall DELAY
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
	ldi r16,16
	add r20,r16
	dec r21
	brne _FILL
	ret	


;######################################################
;Copy right heart branch (1..7 pixels) to left branch (15..9 pixels)
;in: none
;out: none
;uses: r16,R,G,B
BRANCH:
	ldi YL, low(image+3)
	ldi YH, high(image+3)
	ldi XL, low(image+48)
	ldi XH, high(image+48)
	ldi r16,7
_loop:
	ld G,Y+
	ld R,Y+
	ld B,Y+
	st -X,B
	st -X,R
	st -X,G
	dec r16
	brne _loop
	ret

;######################################################
;Writes RGB registers to the bottom pixel of right branch, 
;shifting the branch up.
;! Need to call BRANCH afterwards!
;in: RGB, r16 - start position, r17 - end position
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
	ldi YL, low(image+25)
	ldi YH, high(image+25)
	ldi XL, low(image+27)
	ldi XH, high(image+27)
	ldi r16,24
_loop:
	ld r1,-Y
	st -X,r1
	dec r16
	brne _loop
	st -X,B
	st -X,R
	st -X,G
	ret

;######################################################
;in: R, G, B, r16 - brightness (0-63)
LED_BRIGHTNESS:
	andi r16,0x3f
	mov r17,R
	rcall BRIGHT_BYTE
	mov R,r1
	mov r17,G
	rcall BRIGHT_BYTE
	mov G,r1
	mov r17,B
	rcall BRIGHT_BYTE
	mov B,r1
	ret

BRIGHT_BYTE:
	mov r1,r17
	swap r17
	lsr r17
	lsr r17
	andi r17,0x03
	ldil r2,63
	sub r2,r16
	
_addloop:
	tst r2
	brne _sub
	ret
_sub:
	sub r1,r17
	dec r2
	rjmp _addloop

;##########################################################
;in: RGB - base color, r16 - start offset (bytes), r17 - step
BRANCH_GRADIENT:
	ldi YL, low(image)
	ldi YH, high(image)
	add YL,r16
	ldi r16,15
	;TODO

;##########################################################
;in: RGB - fill color
FILL:
	ldi YL, low(image)
	ldi YH, high(image)
	ldi r16,16
_loop:
	st Y+,G
	st Y+,R
	st Y+,B
	dec r16
	brne _loop
	ret




;######################################################
DELAY_50us:
	ldi r16,0
_loop:
	dec r16
	brne _loop
	ret

;######################################################
;in: r16
DELAY:
	ldi r18,0
_loop1:
	ldi r17,10
_loop:
	dec r17
	brne _loop
	dec r18
	brne _loop1
	dec r16
	brne _loop1
	ret

