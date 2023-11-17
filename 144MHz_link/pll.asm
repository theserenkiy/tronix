;chip m48
.include "C:/avr/includes/m48def.inc"

.equ FOSC = 16000000
.equ BAUDRATE = 9600
.equ UBRR = FOSC/(16*BAUDRATE)-1

.equ DIV_hi = 3
.equ DIV_lo = 0x8614	;freq = 145300
;.equ DIV_lo = 0x8d97	;freq = 144100
;.equ DIV_lo = 0x8259	;freq = 145900
.equ INP_FREQ_PIN = 1;


.macro outi
	ldi r25,@1
	out @0,r25
.endm
.macro stsi
	ldi r25,@1
	sts @0,r25
.endm

.cseg
.org 0
	rjmp RESET
.org 0x00D
	rjmp T1OVF
.org 0x010
	rjmp T0OVF

.org 0x1A

RESET:
	outi DDRB,	0b11110001	;B4 - green LED, B5 - red LED, B0 - varactor
	outi DDRD,0
	sbi PORTB,0
	;stsi PCICR,	0b00000001
	;stsi PCMSK0, 1 << INP_FREQ_PIN

	outi TCCR0A,0
	outi TCCR0B,0b111	;clock src = T0 pin, rising edge
	stsi TIMSK0,1		;ovf int

	stsi TCCR1A,0
	stsi TCCR1B,0b001	;clock src = Fosc
	stsi TCCR1C,0
	stsi TIMSK1,1		;ovf int

	;init UART
	stsi UBRR0H, high(UBRR)
	stsi UBRR0L, low(UBRR)
	; Enable receiver and transmitter
	stsi UCSR0B,(1<<RXEN0)|(1<<TXEN0)
	; Set frame format: 8data, 2stop bit
	stsi UCSR0C,(1<<USBS0)|(3<<UCSZ00)


	clr r24	;cycles of input timer (T0)
	clr r23	;cycles of tacts timer (T1)
	clr r22 ;flag - TIM0OVF occured
	clr r21 ;flag - TIM1OVF occured
	sei


MAIN:

CHECK_TIM0_OVF:
	sbrs r22,0
	rjmp CHECK_TIM1_OVF
	clr r22
	inc r24
	cpi r24,0b00100000	;if we reached 8192
	brne CHECK_TIM1_OVF
	clr r24

	rcall MAKE_PLL

	;rcall DO_EVIL_MATH
	rjmp MAIN

CHECK_TIM1_OVF:
	;rjmp MAIN
	sbrs r21,0
	rjmp  MAIN
	clr r21
	inc r23
	rjmp MAIN

;###################################################
; INTERRUPTS
;##################
;counts input frequency
T0OVF:
	cli
 	lds XL,TCNT1L
	lds XH,TCNT1H
	ldi r22,1
	sei
	reti

;##################
T1OVF:
	ldi r21,1
	reti

;###################################################
; FUNCTIONS
;##################


MAKE_PLL:
	;if we reached measuring interval - do math
	;stopping timers
	outi TCCR0B,0b000	;stop timer 0
	stsi TCCR1B,0b000	;stop timer 1

	;rcall DEBUG_OUT

	ldi r16,low(DIV_lo)
	sub XL,r16
	ldi r16,high(DIV_lo)
	sbc XH,r16
	ldi r16,DIV_hi
	sbc r23,r16
	brmi _pll_down

_pll_up:
	ldi r16,0xff
	rcall UART_TX

	;rcall UART_REGS
	;ldi r16,0xff
	;rcall UART_TX
	rcall UART_REGS
	rcall GET_LOG2

	sbi PORTB,0
	sbi DDRB,0

	;lsl r16
	;lsl r16
	rcall DELAY2
	rjmp _pll_end


_pll_down:
	ldi r16,0
	rcall UART_TX
	ldi r16,0xff
	eor r23,r16
	eor XH,r16
	eor XL,r16
	rcall UART_REGS
	rcall GET_LOG2

	cbi PORTB,0
	sbi DDRB,0
	;lsl r16
	;lsl r16
	lsr r16
	lsr r16
	lsr r16
	lsr r16
	rcall DELAY2
	rjmp _pll_end


_pll_end:
	cbi DDRB,0
	cbi PORTB,0
	;resetting timers
	clr r23
	sts TCNT1H,r23
	sts TCNT1L,r23
	sts TCNT0,r23

	;restarting timers
	outi TCCR0B,0b111	;start timer 0
	stsi TCCR1B,0b001	;start timer 1
	ret


GET_LOG2:
	ldi r16,25
_loop:
	dec r16
	sec
	rol XL
	rol XH
	rol r23
	brcc _loop
_end:
	rcall UART_TX
	mov r17,r16
	clr r16
	rcall UART_TX
	rcall UART_TX
	rcall UART_TX
	mov r16,r17
	ret



DEBUG_OUT:
	;oscilloscope control
	sbi PORTB,5
	ldi r16,0
	rcall DELAY1
	cbi PORTB,5

	rcall UART_REGS
	ret

UART_REGS:
	;output to UART
	mov r16,r23
	rcall UART_TX
	mov r16,XH
	rcall UART_TX
	mov r16,XL
	rcall UART_TX
	ret

;##################
DELAY0:
	ldi r16,0
_loop:
	dec r16
	brne _loop
	ret
;##################
DELAY1:
_loop:
	dec r16
	brne _loop
	ret
;##################
DELAY2:
_init:
	ldi r17,10
_loop:
	dec r17
	brne _loop
	dec r16
	brne _init
	ret
;##################
DELAY3:
	ldi r17,0
	ldi r18,0
_loop:
	dec r17
	brne _loop
	dec r18
	brne _loop
	dec r16
	brne _loop
	ret
;##################
UART_TX:
	; Wait for empty transmit buffer
	lds r17,UCSR0A
	sbrs r17,UDRE0
	rjmp UART_TX
	sts UDR0,r16
	ret
