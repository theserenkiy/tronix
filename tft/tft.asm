;chip m48
.include "m48def.inc"

.equ FOSC = 1000000

.equ TFT_CS = 0
.equ TFT_DC = 1
.equ TFT_RST = 2
.equ TFT_SDA = 3
.equ TFT_SCK = 4

.equ TFT_SPIPORT = PORTB
.equ TFT_SPIDDR = DDRB=
.macro setptr;  regpair_letter, label_address, reg_offset
	ldi @0H, high(@1)
	ldi @0L, low(@1)
	add @0L,@2
	push @2
	ldi @2,0
	adc @0H,@2
	pop @2
.endmacro

.dseg
.org 0x100

.cseg
.org 0

RESET:
	rcall TFT_INIT

MAIN:

	ldi r16, 0x29
	rcall TFT_SEND_CMD
	ldi r16, 0x11
	rcall TFT_SEND_CMD
	ldi r16, 0x21
	rcall TFT_SEND_CMD
	
	ldi r16, 2
	rcall TFT_DELAY
	rjmp MAIN


TFT_INIT:
	sbi TFT_SPIDDR, TFT_CS
	sbi TFT_SPIDDR, TFT_DC
	sbi TFT_SPIDDR, TFT_RST
	sbi TFT_SPIDDR, TFT_SDA
	sbi TFT_SPIDDR, TFT_SCK

	cbi TFT_SPIPORT,TFT_SCK 
	cbi TFT_SPIPORT,TFT_SDA
	sbi TFT_SPIPORT,TFT_CS

	cbi TFT_SPIPORT,TFT_RST
	ldi r16, 1
	rcall TFT_DELAY
	sbi TFT_SPIPORT,TFT_RST
	ldi r16, 2
	rcall TFT_DELAY
	ret


;in: r16
TFT_DELAY:
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


;in: r16
TFT_SEND_CMD:
	cbi TFT_SPIPORT, TFT_DC
	rcall TFT_SEND_BYTE
	ret

;in: r16
TFT_SEND_DATA:
	sbi TFT_SPIPORT, TFT_DC
	rcall TFT_SEND_BYTE
	ret

;in: r16
TFT_SEND_BYTE:
	cbi TFT_SPIPORT, TFT_CS
	ldi r17,8
_loop:
	cbi TFT_SPIPORT, TFT_SCK
	rol r16
	brcs _one
	cbi TFT_SPIPORT, TFT_SDA
	rjmp _data_ok
_one:
	sbi TFT_SPIPORT, TFT_SDA
_data_ok:
	cbi TFT_SPIPORT, TFT_SCK
	dec r17
	brne _loop

	sbi TFT_SPIPORT, TFT_CS
	ret