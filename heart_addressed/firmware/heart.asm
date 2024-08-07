;chip t13
.include "c:/avr/includes/tn13def.inc"

.equ LED_PIN = 4
.equ LEDEN_PIN = 0
.equ MEASURE_PIN = 2
.equ MEASEN_PIN = 1

.equ HUEBITS = 8
.equ HUEMAX = (1 << HUEBITS) - 1
.equ HUESTEP = HUEMAX/6

.equ TEMP_THRESHOLD = 130


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
	rjmp RESET
.org 0x8
	reti

RESET: 
	ldi r16,RAMEND
	out SPL,r16
	sbi DDRB,LED_PIN
	sbi DDRB,LEDEN_PIN
	sbi DDRB,MEASEN_PIN
	
	sbi PORTB, MEASEN_PIN

	;ADC setup
	ldi r16, (1 << ADLAR) | 3
	out ADMUX,r16

	ldi r16, (1 << ADEN) | 0b101
	out ADCSRA, r16

	ldi r16,0
	out ADCSRB, r16
	
	sbi DIDR0,3

	;SLEEP setup
	;sleep en, mode: Power down
	ldi r16, (1 << SE) | (0b10 << SM0)	
	out MCUCR, r16

	;WDT
	ldi r16, (1 << WDTIE) | (0b110 << WDP0)
	out WDTCR, r16

	sei

	ldi r22,0	;temperature pointer
	ldil r3,0	;last avg temp

	cbi PORTB, LEDEN_PIN
	cbi DDRB, LED_PIN
	
MAIN:

	rcall MEASURE
	tst r16
	brne _start
	sleep
	rjmp MAIN

_start:
	sbi PORTB, LEDEN_PIN
	sbi DDRB, LED_PIN
	
	; ldi r16,HUESTEP*4
	; rcall HUE2COLOR
	; ;rcall FILL
	; ;rcall WRITE_LEDS
	; rcall HEARTBEAT
	; ldi r16,250
	; rcall DELAY

	rcall HEARTBEAT_TASK

	cbi PORTB, LEDEN_PIN
	cbi DDRB, LED_PIN

	rcall INIT_MEASURE

	;ldi r16,250
	;rcall DELAY
	sleep

	rjmp MAIN

;#########################################
INIT_MEASURE:
	ldi XH, high(image+27)
	ldi XL, low(image+27)
	ldi r20, 8
_loop:
	st X+, r3
	dec r20
	brne _loop
	ldi r22,0
	ret

;#########################################
HEARTBEAT_TASK:
	;ldi r16,HUESTEP*4
	;rcall HUE2COLOR
	rcall HBT_CYCLE
	rcall HBT_CYCLE
	;rcall HBT_CYCLE
	ret


HBT_CYCLE:
	ldil r9,0
_loop:
	ldi ZL, low(HBT_STEPS*2)
	ldi ZH, high(HBT_STEPS*2)
	add ZL, r9
	ldi r16, 0
	adc ZH, r16
	inc r9
	lpm r16,Z
	tst r16
	brne _show
	ret 
_show:
	rcall HUE2COLOR
	rcall HEARTBEAT
	rjmp _loop
	
HBT_INCR:
	mov r9,r16
	;ldi r16,HUESTEP*4
	rcall HUE2COLOR
	rcall HEARTBEAT
	mov r16,r9
	subi r16,-7
	cpi r16,(HUESTEP*50)/10
	brcs HBT_INCR
	ret

HBT_DECR:
	mov r9,r16
	;ldi r16,HUESTEP*4
	rcall HUE2COLOR
	rcall HEARTBEAT
	mov r16,r9
	subi r16,7
	cpi r16,(HUESTEP*30)/10
	brcc HBT_DECR
	ret 

HBT_STEPS:
.db (HUESTEP*40)/10, (HUESTEP*39)/10
.db (HUESTEP*38)/10, (HUESTEP*37)/10, (HUESTEP*35)/ 10, (HUESTEP*33)/10
.db (HUESTEP*30)/10, (HUESTEP*27)/10, (HUESTEP*24)/10, (HUESTEP*20)/10, (HUESTEP*22)/10, (HUESTEP*24)/10  
.db (HUESTEP*30)/10, (HUESTEP*35)/10
;.db (HUESTEP*36)/10, (HUESTEP*38)/10
.db (HUESTEP*39)/10, (HUESTEP*40)/10
.db (HUESTEP*40)/10, (HUESTEP*41)/10, (HUESTEP*42)/10, (HUESTEP*41)/10;, (HUESTEP*47)/10, (HUESTEP*50)/10
;.db (HUESTEP*43)/10, (HUESTEP*42)/10 
.db (HUESTEP*41)/10, (HUESTEP*40)/10, 0, 0

;#########################################

	;brcs _incr

	rjmp MAIN

.include "drv.asm"




;###########################################
;out: r16 (1 = heated, 0 = passive)
MEASURE:
	;rjmp _turn_on
	sbi PORTB, MEASEN_PIN
	sbi ADCSRA, ADCSRA
_wait:
	sbic ADCSRA, 6
	rjmp _wait
	in r16, ADCL
	in r16, ADCH

	ldi XH, high(image+27)
	ldi XL, low(image+27)
	add XL, r22
	st X, r16
	inc r22
	andi r22,0b00000011
	ldi r18, 0
	ldi r19, 0
	
	ldi XL, low(image+27)
	ldi XH, high(image+27)

	ldi r20, 4
_sum:
	ld r21, X+
	add r18,r21
	ldi r21,0
	adc r19, r21
	dec r20
	brne _sum

	ror r19
	ror r18
	ror r19
	ror r18

	mov r3, r18		;save last avg temp
	sub r18, r16
	;mov r16,r18
	cpi r18, 10
	brcc _turn_off
	cpi r18,4
	brcs _turn_off

	
_turn_on:
	ldi r16,1
	rjmp _end
	
_turn_off:
	ldi r16,0
_end:
	cbi PORTB, MEASEN_PIN
	ret



SHOW_NUMBER:
	ldi YL, low(image)
	ldi YH, high(image)
	ldil r12,8
_loop:
	ldi r17,0
	st Y+,r17
	rol r16
	ror r17
	st Y+,r17
	ldi r17,0
	st Y+,r17
	dec r12
	brne _loop
	ret

FILLNSHOW:
	;rcall FILL
	;rcall WRITE_LEDS
	rcall HEARTBEAT
	ldi r16,100
	rcall DELAY
	ret 


RAINBOW_MOVE:
	ldil r8,HUESTEP*4
_asc:
	mov r16,r8
	subi r16,-2
	cpi r16,HUESTEP*4+4
	brcc _desc
	mov r8,r16
	ldi r17,2
	rcall BRANCH_RAINBOW
	rcall BRANCH
	rcall WRITE_LEDS
	ldi r16,100
	rcall DELAY
	rjmp _asc
_desc:
	mov r16,r8
	subi r16,2
	cpi r16,HUESTEP*4-70
	brcs _asc
	mov r8,r16
	ldi r17,2
	rcall BRANCH_RAINBOW
	rcall BRANCH
	rcall WRITE_LEDS
	ldi r16,100
	rcall DELAY
	rjmp _desc
	
_end:
	ret


;######################################################
;RGB - color, r16 - speed (delay), r17 - brightness to stop at
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
;in: r16 - starting hue, r17 - hue step
;out: none
;uses: r16, r20, r21
BRANCH_RAINBOW:
	ldi YL, low(image)
	ldi YH, high(image)
	mov r12,r16
	mov r11,r17
	ldi r18,9
_FILL:
	mov r16,r12
	rcall HUE2COLOR
	st Y+,G
	st Y+,R
	st Y+,B
	add r12,r11
	dec r18
	brne _FILL
	mov r16,r12
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

