
TEST_CYCLE:
	cli
	outi TCCR0B,0b011	; T/64
	outi TCNT0,0
	ldi r24,1
	sbi PORTB,PIN_LED
	sei
_wait:
	tst r24
	brne _wait
	;cli
	cbi PORTB,PIN_LED
	outi TCCR0B,0b010	; parking timer
	sleep
	ret



RX_WAIT:	;r16 - n cycles; r17 - return
	ldi r17,0
	acomp_on 1
_loop:
	sbrc r17,0
	rjmp _ok
	dec r16
	brne _loop
	ret
_ok:
	drain_cap
	ret




	RX_NOIRQ:
		drain_cap
		acomp_on 0
		ldi r17,0
	_loop:
		sbic ACSR, ACO
		rjmp _ok
		dec r16
		brne _loop
		ret
	_ok:
		ldi r17,1
		drain_cap
		ret



	RX:
		sbi PORTB,PIN_DIVIDER
		drain_cap
		ldi r16,0
		rcall RX_WAIT
		sbrc r17,0
		rcall RECEIVED
		cbi PORTB,PIN_DIVIDER
		ret


RECEIVED:
	;ldi r16,1
	;rcall RX_WAIT
	;sbrc r17,0
	;ret

	sbi PORTB,PIN_LED
	nop
	nop
	nop
	cbi PORTB,PIN_LED
	ret
