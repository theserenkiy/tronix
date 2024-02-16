;######################################################
;Places R G B registers to array `image` at pointer Y
RGB2MEM:

	st Y+,G
	st Y+,R
	st Y+,B
	ret

;##########################################################
;in:	r16 - hue
;out:	R,G,B
;uses:	r16

HUE2COLOR:
	ldi R,0
	ldi G,0
	ldi B,0
	andi r16,HUEMAX
	cpi r16,HUESTEP
	brcc _p2
	ldi G, HUESTEP
	mov B,r16
	rjmp _end
_p2:
	cpi r16,HUESTEP*2
	brcc _p3
	ldi B,HUESTEP
    ldi G,HUESTEP*2
	sub G,r16
	rjmp _end
_p3:
	cpi r16,HUESTEP*3
	brcc _p4
	ldi B,HUESTEP
	mov R,r16
	subi R,HUESTEP*2
	rjmp _end
_p4:
	cpi r16,HUESTEP*4
	brcc _p5
	ldi R,HUESTEP
	ldi B,HUESTEP*4
	sub B,r16
	rjmp _end
_p5:
	cpi r16,HUESTEP*5
	brcc _p6
	ldi R,HUESTEP
	mov G,r16
	subi G,HUESTEP*4
	rjmp _end
_p6:
	cpi r16,HUESTEP*6
	brcc _p7
	ldi G,HUESTEP
	ldi R,HUESTEP*6
	sub R,r16
	rjmp _end
_p7:
	ldi G,HUESTEP
	ldi B,1
_end:
	lsl R
	lsl G
	lsl B
    lsl R
	lsl G
	lsl B
	ret

;##########################################################
;writes the array `image` to LEDs, shifting each value 4 times right
;uses: r16, r17, r18
WRITE_LEDS:
	ldi r16, 48		;byte coubter
	ldi YL, low(image)
	ldi YH, high(image)
	
_dump_byte:
	ld r17,Y+		;byte value
	ldi r18,8		;bit counter
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
	cpi r18,6
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