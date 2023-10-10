;chip m48
.include "C:/avr/includes/m48def.inc"

.equ FOSC = 16000000
.equ BAUDRATE = 9600
.equ UBRR = FOSC/(16*BAUDRATE)-1
.equ SDA = 3
.equ SCL = 4
.equ I2C_ADDR = 0x60

.macro outi
	ldi r25,@1
	out @0,r25
.endm
.macro stsi
	ldi r25,@1
	sts @0,r25
.endm
.macro SDA_hi
	cbi DDRD,SDA
	rcall I2C_delay
.endm
.macro SDA_lo
	sbi DDRD,SDA
	rcall I2C_delay
.endm
.macro SCL_hi
	cbi DDRD,SCL
	rcall I2C_delay
.endm
.macro SCL_lo
	sbi DDRD,SCL
	rcall I2C_delay
.endm
.macro I2C_clk
	SCL_hi
	SCL_lo
.endm
.macro I2C_write_reg
	ldi r16,@0
	ldi r17,@1
	rcall I2C_write_2bytes
.endm

.macro I2C_read_reg
	ldi r16,@0
	rcall I2C_read
.endm

.cseg
.org 0
	rjmp RESET

.org 0x1A


RESET:
	outi SPH,high(RAMEND)
	outi SPL,low(RAMEND)

	cbi PORTD,SDA
	cbi PORTD,SCL
	sbi DDRB,3 ;LED

.include "si5351.regs.asm"

MAIN:
	;ldi r16,0b10101010
	;ldi r17,0b11001100
	;rcall I2C_write_2bytes
	;I2C_write_reg 3,0b11001100
	;ldi r16,3
	;rcall DELAY2
	;I2C_read_reg 3
	;ldi r16,6
	;rcall DELAY2
	rjmp MAIN


I2C_read:	;r16 - register addr to read
	rcall I2C_write_1byte
	ldi r16,1
	rcall I2C_start
	rcall I2C_receive_byte
	SDA_hi
	SCL_hi
	SCL_lo
	rcall I2C_stop
	ret


I2C_write_1byte:	;r16
	push r16
	ldi r16,0
	rcall I2C_start
	pop r16
	rcall I2C_send_byte
	rcall I2C_stop
	ret

I2C_write_2bytes:	;r16, r17
	push r17
	push r16
	ldi r16,0
	rcall I2C_start
	pop r16
	rcall I2C_send_byte
	pop r16
	rcall I2C_send_byte
	rcall I2C_stop
	ret

I2C_receive_byte:
	ldi r17,8
	ldi r16,0
_loop:
	SCL_hi
	sbic PIND,SDA
	rjmp _is_1
_is_0:
	clc
	rjmp _save
_is_1:
	sec
_save:
	rol r16
	SCL_lo
	dec r17
	brne _loop
	ret

I2C_send_byte:	;r16
	ldi r17,8
_loop:
	rol r16
	brcc _send0
_send1:
	SDA_hi
	rjmp _tact
_send0:
	SDA_lo
_tact:
	I2C_clk
	dec r17
	brne _loop
	SDA_hi
	SCL_hi
	ldi r16,0
	sbic PIND,SDA
	ldi r16,1	;ACK error flag
	SCL_lo
	rcall I2C_delay
	rcall I2C_delay

	tst r16
	brne _ACK_error
	;rcall LED_short
	rjmp _end

_ACK_error:
	;rcall LED_long
_end:
	ret


I2C_start:
	SDA_lo
	SCL_lo
	ldi r17,I2C_ADDR
	lsl r17
	or r16,r17
	rcall I2C_send_byte
	ret

I2C_stop:
	SDA_lo
	SCL_hi
	SDA_hi
	rcall I2C_delay
	rcall I2C_delay
	rcall I2C_delay
	rcall I2C_delay
	rcall I2C_delay
	rcall I2C_delay
	ret

I2C_delay:
	ldi r18,20
_loop:
	dec r18
	brne _loop
	ret

LED_long:
	sbi PORTB,3
	ldi r16,30
	rcall DELAY
	cbi PORTB,3
	ret

LED_short:
	sbi PORTB,3
	ldi r16,1
	rcall DELAY
	cbi PORTB,3
	ret


DELAY1:
_loop:
	dec r16
	brne _loop
	ret

DELAY2:
	ldi r17,0
_loop:
	dec r17
	brne _loop
	dec r16
	brne _loop
	ret

DELAY:
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
