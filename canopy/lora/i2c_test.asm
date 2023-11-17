;chip atmega8

.include "m8def.inc"
.include "macros.inc.asm"

.equ F_OSC = 1000000
.equ BEEP_OSC = F_OSC/64
.equ UART_SPEED = 57600

.equ IN_MATCH = 4
.equ IN_GNRMC = 5
.equ LAT_OK = 6
.equ LON_OK = 7


.dseg

latitude: .byte 12
longitude: .byte 12
parser_flags: .byte 1
parser_pos: .byte 1

.cseg
.org 0

rjmp RESET ; Reset Handler
reti;rjmp EXT_INT0 ; IRQ0 Handler
reti;rjmp EXT_INT1 ; IRQ1 Handler
reti;rjmp TIM2_COMP ; Timer2 Compare Handler
reti;rjmp TIM2_OVF ; Timer2 Overflow Handler
reti;rjmp TIM1_CAPT ; Timer1 Capture Handler
reti;	rjmp TIM1_COMPA ; Timer1 CompareA Handler
reti;rjmp TIM1_COMPB ; Timer1 CompareB Handler
reti;rjmp TIM1_OVF ; Timer1 Overflow Handler
reti;rjmp TIM0_OVF ; Timer0 Overflow Handler
reti;rjmp SPI_STC ; SPI Transfer Complete Handler
reti;rjmp USART_RXC ; USART RX Complete Handler
reti;rjmp USART_UDRE ; UDR Empty Handler
reti;rjmp USART_TXC ; USART TX Complete Handler
reti;rjmp ADC ; ADC Conversion Complete Handler
reti;rjmp EE_RDY ; EEPROM Ready Handler
reti;rjmp ANA_COMP ; Analog Comparator Handler
reti;rjmp TWSI ; Two-wire Serial Interface Handler
reti;rjmp SPM_RDY ; Store Program Memory Ready Handler

.include "core.inc.asm"
.include "uart.inc.asm"
.include "twi2.asm"

;########################################################
RESET:

	outi SPL,low(RAMEND)
	outi SPH,high(RAMEND)

	;UART init
	.equ UBR_VAL = F_OSC/(16*UART_SPEED)-1
	outiw UBRR,UBR_VAL
	outi UCSRB,(1 << TXEN) | (1 << RXEN)
	outi UCSRC,(1<<URSEL)|(1<<USBS)|(3<<UCSZ0)	;frame: 8 data 1 stop

	outi TWBR,2
	outi TWSR,0b00

	rcall TWI_INIT

	stsi twi_sla,0x42 << 1
	twi_add 0xFF
	ldi temp,0

	rcall TWI_RUN

	rjmp LOOP


; $GNRMC,110452.00,A,6002.35599,N,03024.91883,E,1.052,,090719,,,A*64
;temp - current byte of stream

PARSE_CMD:
	;push temp
	lds temp1,parser_flags
	;if we inside match
	;(previously parsed bytes corresponds given string at pointer Z)
	sbrc temp1,IN_MATCH
	rjmp PARSER_IN_MATCH
	ldiw Z,GNRMS*2
	rjmp PARSE_NEXT_BYTE

PARSER_IN_MATCH:
	sbrc temp1,IN_CMD
	rjmp PARSER_IN_CMD
	rjmp PARSE_NEXT_BYTE

PARSE_NEXT_BYTE:
	;parse next byte
	lpm
	adiw ZH:ZL,1
	mov temp2,r0
	tst temp2
	breq PARSE_CMD_END
	cp temp,temp2
	breq PARSE_MATCH
	cbr temp1,(1 << IN_MATCH)
	sts parser_flags,temp1
	ret

PARSE_MATCH:
	sbr temp1,(1 << IN_MATCH)
	sts parser_flags,temp1
	ret

PARSE_CMD_END:
	sbr temp1,(1 << IN_CMD)
	sts parser_flags,temp1
	ret

GNRMS: .db "$GNRMS"

READ_DEVICE:
	uart_byte 'D'
	uart_byte 10
	rcall TWI_START
	brne RBERR
	lds temp,twi_sla
	ori temp,1
	rcall TWI_SEND_SLA
	brne RBERR

	ldi temp,1
READ_LOOP:
	push temp
	rcall TWI_READ
	pop temp1
	brne RBERR
	tst temp1
	breq READ_USER
	uart_reg temp
	cpi temp,0xFF
	ldi temp,0
	breq READ_LOOP
	ldi temp,1
	rjmp READ_LOOP


RBERR:
	uart_byte 'R'
	uart_byte 'B'
	uart_byte 'E'
	uart_byte 10
	rjmp LOOP

ERR:
	in temp,SREG
	uart_hex temp
	uart_byte ':'
	uart_byte '-'
	uart_byte '('
	uart_byte 10
	rjmp LOOP


READ_USER:
	rcall TWI_STOP
	;uart_byte 'U'
	;uart_byte 10
	ldiw Y,twi_data
READ_USER_BYTE:
	rcall UART_RX
	cpi temp,0x2E
	breq END_USER_INPUT
	st Y+,temp
	uart_reg temp
	rjmp READ_USER_BYTE

END_USER_INPUT:
	clr temp
	st Y+,temp

	rcall TWI_START
	brne RBERR
	lds temp,twi_sla
	rcall TWI_SEND_SLA
	brne RBERR

	ldiw Y,twi_data

SEND_USER_BYTE:

	ld temp,Y+
	uart_byte 'S'
	uart_hex temp


	tst temp
	breq READ_USER_END
	rcall TWI_WRITE
	rjmp SEND_USER_BYTE

READ_USER_END:
	ldi temp,10
	rcall TWI_WRITE
	rcall TWI_STOP
	rjmp READ_DEVICE



	;ldi temp,0
	;rcall TWI_RUN

	rjmp LOOP




LOOP:
	nop
	nop
	nop
	nop
	nop
	rjmp LOOP
