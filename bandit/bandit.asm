;chip m48
.include "m48def.inc"

.equ DATA = 0
.equ CLK = 2
.equ CS = 1
.equ SPIPORT = PORTC
.equ SPIDDR = DDRC

.macro disp_cmd
	ldi r16, @0
	ldi r17, @1
	rcall SEND_CMD
.endmacro

.macro setptr;  regpair_letter, label_address, reg_offset
	ldi @0H, high(@1)
	ldi @0L, low(@1)
	add @0L,@2
	ldi @2,0
	adc @0H,@2
.endmacro

.dseg
.org 0x100
display:	.byte 32
positions:	.byte 12	;6 pairs (sym_num, left_shift)

.cseg
.org 0

RESET:
	sbi SPIDDR, DATA		
	sbi SPIDDR, CLK
	sbi SPIDDR, CS
	cbi SPIPORT,CLK 
	cbi SPIPORT,DATA 
	sbi SPIPORT,CS
	
	sbi DDRB,0
	sbi DDRB,1

	rcall DISP_INIT

MAIN:
	ldi r24,0
_loop:
	ldi r16,1
	ldi XH, high(display)
	ldi XL, low(display)
	ldi r16,0
	mov r17,r24
	rcall WRITE_SYM_TO_MEM_LSHIFT
	ldi XH, high(display)
	ldi XL, low(display)
	ldi r16,1
	ldi r17,8
	sub r17,r24
	inc r17
	rcall WRITE_SYM_TO_MEM_RSHIFT

	rcall SEND_MEMORY
	rcall DELAY
	
	inc r24
	cpi r24,9
	brne _loop

	rjmp MAIN


;in = r16
SEND_BYTE:
	ldi r17,8
	mov r3,r16
_loop:
	rol r3
	brcs _send1
	cbi SPIPORT,DATA
	rjmp _data_ok
_send1:
	sbi SPIPORT,DATA
_data_ok:
	rcall DELAY1
	sbi SPIPORT,CLK
	rcall DELAY1
	cbi SPIPORT,CLK 
	rcall DELAY1
	dec r17
	brne _loop
	ret


;in: r16 - high, r17 - low
SEND_CMD:
	mov r0,r16
	mov r1,r17
	cbi SPIPORT, CS
	rcall DELAY1
	ldi r18,4

_loop:	
	mov r16,r0
	rcall SEND_BYTE
	mov r16,r1
	rcall SEND_BYTE
	dec r18
	brne _loop

	sbi SPIPORT, CS
	rcall DELAY1
	ret


;void
SEND_MEMORY:
	ldi r16,0
	mov r20,r16
_loop8:
	mov r16, r20
	setptr X, display, r16
	cbi SPIPORT, CS
	rcall DELAY1
	ldi r16,0
	mov r21,r16
_loop4:
	mov r16,r20
	inc r16
	rcall SEND_BYTE
	ld r16,X
	rcall SEND_BYTE
	
	ldi r16,8
	add XL,r16
	ldi r16,0
	adc XH,r16
	inc r21
	cpi r21,4
	brne _loop4
;end loop4
	sbi SPIPORT, CS
	rcall DELAY1
	inc r20
	cpi r20,8
	brne _loop8
;end loop8
	ret



;in: X - start mem addr to write, r16 - sim num, r17 - lshift
WRITE_SYM_TO_MEM_LSHIFT:
	lsl r16
	lsl r16
	lsl r16
	add r16,r17
	setptr Z, IMAGES*2, r16
	ldi r18, 8
	sub r18,r17
_loop:
	lpm r16, Z+
	st X+, r16
	dec r18
	brne _loop

	ret



;in: X - start mem addr to write, r16 - sim num, r17 - lshift
WRITE_SYM_TO_MEM_RSHIFT:
	lsl r16
	lsl r16
	lsl r16
	setptr Z, IMAGES*2, r16
	ldi r18, 8
	sub r18,r17
	add XL, r17
	ldi r17,0
	adc XH, r17
_loop:
	lpm r16, Z+
	st X+, r16
	dec r18
	brne _loop

	ret




;in: r16 - position 0 to 3
WRITE_POSITION:
	lsl r16
	setptr X, positions, r16
	ld r16, X+
	ld r17, X+
	ret



DISP_INIT:
	disp_cmd 0x0C, 0x01		;turn on
	disp_cmd 0x09, 0x00		;no decode
	disp_cmd 0x0A, 0x00		;brightness = 1/2
	disp_cmd 0x0B, 0x07		;scan limit = 8
	disp_cmd 0x0F, 0x00		;display test mode
	ret



DELAY1:
	ldi r16,20
_loop:
	dec r16
	brne _loop
	ret


DELAY:
	ldi r16,0
	ldi r17,0
	ldi r18,5
_loop:
	dec r16
	brne _loop
	dec r17
	brne _loop
	dec r18
	brne _loop
	ret



IMAGES:
;berry
.db 0x08, 0x10, 0x7c, 0xd6, 0xea, 0x7c, 0x38, 0x10
;seven
.db 0x7f, 0x63, 0x46, 0x0c, 0x18, 0x38, 0x38, 0x38		;no transp
;.db 0x00, 0xe0, 0xc7, 0x8f, 0x9f, 0xb0, 0xe0, 0xc0
;smiley
.db 0x3c, 0x7e, 0xdb, 0xff, 0xbd, 0xdb, 0x66, 0x3c
;heart
.db 0x00, 0x36, 0x49, 0x41, 0x22, 0x14, 0x08, 0x00


