;chip m328p
.include "C:\avr\includes\m48def.inc"

.equ FOSC = 16000000
.equ UART_BAUDRATE = 9600
.equ UBRR = FOSC/(16*UART_BAUDRATE)-1
.equ ADMUX_INITIAL = 0b01100010

.macro outi
	ldi r25,@1
	out @0,r25
.endm
.macro stsi
	ldi r25,@1
	sts @0,r25
.endm

.dseg
.org 0x100
servo: .byte 3


.cseg
	rjmp RESET

.org 0x30

RESET:
	outi DDRB, 0b00000111
	outi DDRC, 0b00000011
	stsi DIDR0, 0b00001100

	cbi PORTC,0
	sbi PORTC,1

	stsi UBRR0H,high(UBRR)
	stsi UBRR0L,low(UBRR)
	stsi UCSR0B,(1<<RXEN0)|(1<<TXEN0)
	stsi UCSR0C,(1<<USBS0)|(3<<UCSZ00)

MAIN:
	sbi PORTB,0
	sbi PORTB,1
	sbi PORTB,2
	rcall DELAY_1ms

	rcall PWM

	cbi PORTB,0
	cbi PORTB,1
	cbi PORTB,2

	ldi r16,ADMUX_INITIAL
	rcall ADC_CONVERT
	sts servo,r16

	ldi r16,ADMUX_INITIAL+1
	rcall ADC_CONVERT
	sts servo+1,r16

	ldi r16,0
	sbic PINC,4
	ldi r16,250
	sts servo+2,r16

	rcall DELAY_18ms

	rjmp MAIN


;########################
ADC_CONVERT:
	sts ADMUX, r16
	stsi ADCSRA,0b11000100
	stsi ADCSRB,0
_wait:
	lds r16,ADCSRA
	sbrc r16,ADSC
	rjmp _wait

	lds r16,ADCH
	rcall UART_TX
	ret

;########################
PWM:
	ldi r17,0
	ldi r21,2
	lds r18,servo
	lds r19,servo+1
	lds r20,servo+2
_loop:
	cp r17,r18
	brlo _n1
	cbi PORTB,0
_n1:
	cp r17,r19
	brlo _n2
	cbi PORTB,1
_n2:
	cp r17,r20
	brlo _n3
	cbi PORTB,2
_n3:
	rcall DELAY_1_128_ms
	add r17,r21
	brcc _loop
	ret

;########################
DELAY_1_128_ms:
	ldi r16,(FOSC/600/128/3)
_loop:
	dec r16
	brne _loop
	ret

;########################
DELAY_1ms:
	ldi r17,FOSC/500000
_loop1:
	;ldi r16,167
	ldi r16,120
_loop:
	dec r16
	brne _loop
	dec r17
	brne _loop1
	ret

;########################
DELAY_18ms:
	ldi r18,20
_loop:
	rcall DELAY_1ms
	dec r18
	brne _loop
	ret

UART_TX:
	lds r17,UCSR0A
	sbrs r17,UDRE0
	rjmp UART_TX
	sts UDR0,r16
	ret
