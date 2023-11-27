;chip m48
.include "m48def.inc"
.include "macros.inc.asm"

.equ BAUDRATE = 1200
;.equ FOSC = 20000000
.equ FOSC = 16000000
.equ LED = 3; PB
.equ SPEAKER = 5; PD
.equ POT_PLUS = 5; PB
.equ LED_MASK = 1 << LED

;lcd pins on PORTB
.equ LCD_A0 = 4
.equ LCD_E = 5
.equ LCD_DATA_PORT = PORTB
.equ LCD_SERVICE_PORT = PORTB
.equ LCD_DATA_LOW_BIT = 0


.equ DS18_PORT = PORTD
.equ DS18_PIN = 3
.equ DS18_RESOLUTION = 12

.dseg

;.org 0

cursor: .byte 1

.cseg
.org 0
	rjmp RESET


.include "delays.asm"
.include "lcd1602.asm"
.include "ds18b20.asm"
.include "m48_uart.asm"
.include "decimal_16.lib.asm"

.equ STDOUT = UART_TX

RESET:
	init_stack_pointer
	rcall UART_init
	outi DDRB,0xff
	outi PORTB,0
	delay_ms 20

	rcall LCD_INIT
	rcall DS18_INIT

	print "Preved!\"

	rjmp MAIN_LOOP




MAIN_LOOP:
	;rjmp MAIN_LOOP

	lcd_cursor 0,0

	;rcall DS18_START_CONVERSION

	;rdstr "Reading temp \",LCD_DATA
	;lcd_cursor 0,0

;AWAIT_DATA:
;	rcall DS18_IS_DATA_READY
;	brcc AWAIT_DATA

	rcall DS18_MEASURE
	;delay_ms 750
	;rcall DS18_READ_DATA

	lds r16,DS18_DATA+1  ;MSB
	andi r16, 0b00000111
	;rcall UART_TX
	mov R17,R16

	lds r16,DS18_DATA    ;LSB
	;rcall UART_TX

	ror r17
	ror r16
	ror r17
	ror r16
	ror r17
	ror r16
	ror r17
	ror r16

	;uart_byte 0x88
	;rcall UART_TX

	ldi r17,0
	rcall CONVERT_TO_DECIMAL_16
	rcall LCD_NUMBER_2_0

	;rjmp MAIN_DELAY

	lcd_data '.'

	lds r16,DS18_DATA  ;LSB
	andi r16, 0b00001111
	;ldi r16,0b10171
	;lsr r16
	rcall UART_TX
	rcall CONVERT_FRACTION
	rcall LCD_NUMBER_3_1

	;print "              \";

	;delay_ms 500

MAIN_DELAY:
	delay_ms 100

	rjmp MAIN_LOOP


;********************************************
;********************************************
;********************************************

;STDOUT:
;	rcall UART_TX
	;rcall LCD_DATA
;	ret


;input - memory DECIMAL_16 x3
LCD_NUMBER_2_0:
	ldiw Y,DECIMAL_16+3
	repeat LCD_DIGIT,3
	ret

LCD_NUMBER_3_1:
	ldiw Y,DECIMAL_16+4
	repeat LCD_DIGIT,3
	ret
