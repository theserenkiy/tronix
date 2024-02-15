;##########################################################
;writes the array `image` to LEDs
;uses: r16, r17, r18
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