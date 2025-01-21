;chip m48
.include "m48def.inc"

.equ DATA = 0
.equ CLK = 2
.equ CS = 1
.equ SPIPORT = PORTC
.equ SPIDDR = DDRC

.equ DRUMS_COUNT = 4
.equ IMAGES_COUNT = 10
.equ EMPTY_LINES = 2

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
positions:	.byte DRUMS_COUNT * 4	;0: sym_num (0 .. IMAGES_COUNT-1)
									;1: shift (0 .. 9)]
									;2: increment
									;3: value


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

	;clear positions memory
	ldi XH, high(positions)
	ldi XL, low(positions)
	ldi r16,0
	ldi r17,4
	ldi r18,0
_loop:
	st X+,r18
	st X+,r17
	st X+,r16
	st X+,r16
	inc r18
	cpi r18,DRUMS_COUNT
	brne _loop

	;rcall START_DRUMS

MAIN:
	ldi r24,0
	
_loop:
	mov r16,r24
	rcall UPDATE_DRUM

	inc r24
	cpi r24,DRUMS_COUNT
	brne _loop

	rcall SEND_MEMORY
	rcall DELAY

	ldi r16,1
	rcall INC_POSITION

	;rcall MK_TICK

	rjmp MAIN


START_DRUMS:
	ldi XH, high(positions)
	ldi XL, low(positions)
	ldi r18,DRUMS_COUNT
	ldi r17,130				;RANDOMABLE
_loop:
	ld r16,X+
	ld r16,X+
	subi r17,13				;RANDOMABLE
	st X+, r17
	st X+, r17
	dec r18
	brne _loop
	ret



MK_TICK:
	ldi XH, high(positions)
	ldi XL, low(positions)
	ldi r20,0
	ldi r21,0
_loop:
	ld r16, X+
	ld r17, X+
	ld r18, X+
	ld r19, X+
	tst r18
	breq _skip_inc
	add r19,r18
	brcc _skip_inc
	
	cpi r17,0
	brne _decshift
	ldi r17,8+EMPTY_LINES
	cpi r16,0
	brne _decdrum
	ldi r16, IMAGES_COUNT-1
	rjmp _decshift
_decdrum:
	dec r16
_decshift:
	dec r17

_skip_inc:
	subi r18,7				;RANDOMABLE
	st -X,r19
	st -X,r18

	inc r20
	cpi r20,DRUMS_COUNT
	brne _loop
	ret

;in: r16 - n drum
INC_POSITION:
	lsl r16
	lsl r16
	setptr X, positions, r16
	ld r16,X+
	ld r17,X+

	cpi r17,0
	brne _decshift
	ldi r17,8+EMPTY_LINES
	cpi r16,0
	brne _decdrum
	ldi r16, IMAGES_COUNT-1
	rjmp _decshift
_decdrum:
	dec r16
_decshift:
	dec r17
	st -X,r17
	st -X,r16
	ret


;in: r16 - n drum
UPDATE_DRUM:
	mov r20, r16
	lsl r16		;r16 *= 2
	setptr X, positions, r16
	ld r16, X+
	ld r17, X

	lsl r20
	lsl r20
	lsl r20		;r20 *= 8
	setptr X, display, r20
	rcall WRITE_SYMS_SHIFTED_
	ret

;in: X - ram pointer
;r16 - sym num
;r17 - offset 0..9
WRITE_SYMS_SHIFTED_:
	ldi r18,0	;row counter 0..7
	lsl r16
	lsl r16
	lsl r16

	mov r19,r16
	cpi r17,8
	brcc _empty
	add r16,r17
	setptr Z, IMAGES*2, r16
_loop0:
	lpm r16, Z+
	st X+,r16
	inc r18
	inc r17
	cpi r17,8
	brne _loop0
_empty:
	ldi r16,0
_loop1:
	cpi r18,8
	breq _end
	cpi r17,8+EMPTY_LINES
	breq _lowsym
	st X+,r16
	inc r17
	inc r18
	rjmp _loop1
_lowsym:
	subi r19,-8
	setptr Z, IMAGES*2, r19
_loop2:
	lpm r16, Z+
	st X+, r16
	inc r18
	cpi r18,8
	brne _loop2
_end:
	ret



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


;in: X - start mem addr to write
;r16 - sim num
;r17 - start byte of symbol
;r18 - bytes to read
WRITE_SYM_PART_TO_MEM:
	tst r18
	breq _end
	cpi r18,8
	brcc _end
	lsl r16		;r16 *= 8
	lsl r16
	lsl r16
	add r16, r17
	setptr Z, IMAGES*2, r16
_loop:
	lpm r16,Z+
	st X+, r16
	dec r18
	brne _loop
_end:
	ret

WRITE_EMPTY_ROW:
	ldi r16,0
	st X+, r16
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
	ldi r16,1
_loop:
	dec r16
	brne _loop
	ret


DELAY:
	ldi r16,0
	ldi r17,100
	;ldi r18,2
_loop:
	dec r16
	brne _loop
	dec r17
	brne _loop
	;dec r18
	;brne _loop
	ret


IMAGES:
.db 0x3c, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3c	;0
.db 0x18, 0x38, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3c	;1
.db 0x3c, 0x66, 0x06, 0x0c, 0x18, 0x30, 0x60, 0x7e	;2
.db 0x3c, 0x66, 0x06, 0x1c, 0x06, 0x06, 0x66, 0x3c	;3
.db 0x06, 0x0e, 0x16, 0x26, 0x66, 0x7f, 0x06, 0x06	;4
.db 0x7e, 0x60, 0x60, 0x7c, 0x06, 0x06, 0x66, 0x3c	;5
.db 0x3c, 0x62, 0x60, 0x7c, 0x66, 0x66, 0x66, 0x3c	;6
.db 0x7e, 0x06, 0x0c, 0x18, 0x30, 0x30, 0x30, 0x30	;7
.db 0x3c, 0x66, 0x66, 0x3c, 0x66, 0x66, 0x66, 0x3c	;8
.db 0x3c, 0x66, 0x66, 0x66, 0x3e, 0x06, 0x66, 0x3c	;9
.db 0x3c, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3c	;0


IMAGES_:
;berry
.db 0x08, 0x10, 0x10, 0x7c, 0xfe, 0xfe, 0x7c, 0x38
;seven
.db 0x7f, 0x63, 0x46, 0x0c, 0x18, 0x38, 0x38, 0x38
;devil
.db 0x42, 0x66, 0xff, 0xdb, 0xff, 0x42, 0x3c, 0x18
;smiley
.db 0x3c, 0x7e, 0xdb, 0xff, 0xbd, 0xdb, 0x66, 0x3c
;heart
;.db 0x00, 0x36, 0x49, 0x41, 0x22, 0x14, 0x08, 0x00
.db 0x00, 0x36, 0x7f, 0x7f, 0x3e, 0x1c, 0x08, 0x00
;berry: the first should be copied to last
.db 0x08, 0x10, 0x10, 0x7c, 0xfe, 0xfe, 0x7c, 0x38


