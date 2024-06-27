;chip t2313
.include "C:/avr/includes/tn2313def.inc"

.equ FCLK = 12000000

;ON PORTB:
.equ RS = 5
.equ E = 4
;PB3...PB0 => D7...D4 of LCD
;RW pin of LCD connected to GND

.macro wait_us
	ldi r16, @0
	rcall DELAY_US
.endmacro

.macro wait_ms
	ldi r16, @0
	rcall DELAY_MS
.endmacro

.macro lcd_cmd
	ldi r16, @0
	rcall LCD_CMD_8B
.endmacro

.macro lcd_cmd_4
	ldi r16, @0
	rcall LCD_CMD_4B
.endmacro

.macro lcd_data
	ldi r16, @0
	rcall LCD_WRITE
.endmacro

.macro print; @0 - label to string .db
	ldi ZH, high(@0 *2)
	ldi ZL, low(@0 *2)
	rcall LCD_PRINT_STR
.endmacro

.macro setpos; @0 - x (0..15), @1 - y (0,1)
	ldi r16, @0 | (@1 << 6) | (1 << 7)
	rcall LCD_CMD_8B
.endmacro



RESET:
	ldi r16, 0xff
	out DDRB, r16

MAIN:
	rcall LCD_INIT

	setpos 0, 0
	print STR1

	setpos 4, 1
	print STR2

	rjmp PC


STR1:
.db "Vse deti -\0"

STR2:
.db "porosyata!\0"


;#########################################
LCD_INIT:

	wait_ms 100				;wait after power on

	lcd_cmd_4 0b00000011	;do some magic...
	wait_ms 10
	lcd_cmd_4 0b00000011
	wait_ms 1
	lcd_cmd_4 0b00000011
	wait_ms 1
	lcd_cmd_4 0b00000010	;set 4-bit mode
	wait_ms 1

	lcd_cmd 0b00101100		;4 bit mode, 2 lines, 5x8
	lcd_cmd 0b00001111		;displ on, cursor on, cursor blinking
	lcd_cmd 0b00000001		;clear display
	wait_ms 10

	lcd_cmd 0b00000111

	ret

;#########################################
;prints string to LCD
;Z - address of string
LCD_PRINT_STR:
_loop:
	lpm r16,Z+
	tst r16
	breq _end
	rcall LCD_WRITE
	rjmp _loop

_end:
	ret

;#########################################
;writes 4-bits to LCD, generating clock on E pin
;r16 - data (only lower 4 bits used)
LCD_WRITE_NIBBLE:
	in r17,PORTB
	andi r17, 0xF0
	andi r16, 0x0F
	or r16,r17
	out PORTB, r16
	sbi PORTB, E
	wait_ms 1
	cbi PORTB, E
	wait_ms 1
	ret

;#########################################
;writes only 4 lower bits of r16 to LCD
;r16 - cmd
LCD_CMD_4B:
	cbi PORTB, RS
	rcall LCD_WRITE_NIBBLE	
	sbi PORTB, RS
	ret

;#########################################
;writes command (r16) to LCD in 4-bit mode
;r16 - cmd
LCD_CMD_8B:
	cbi PORTB, RS
	rcall LCD_WRITE
	sbi PORTB, RS
	ret

;#########################################
;writes data (r16) to LCD in 4-bit mode
;higher 4 bits first, then lower 4 bits
;r16 - data
LCD_WRITE:
	mov r20,r16
	swap r16
	rcall LCD_WRITE_NIBBLE
	mov r16, r20
	rcall LCD_WRITE_NIBBLE
	ldi r16,1
	rcall DELAY_MS
	ret 


;#########################################
;r16 - us to wait
DELAY_US:
_loop:
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	dec r16
	brne _loop
	ret	


;#########################################
;r16 - ms to wait
DELAY_MS:
	ldi r17,0
_loop:
	nop
	dec r17
	brne _loop
	dec r16
	brne _loop
	ret	
