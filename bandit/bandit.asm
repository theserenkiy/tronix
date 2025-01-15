.include "m48def.inc"

.equ DATA = 1
.equ CLK = 2
.equ SPIPORT = PORTD

RESET:
    sbi SPIPORT-1,DATA
    sbi SPIPORT-1,CLK
    cbi SPIPORT,CLK 
    cbi SPIPORT,DATA 

;in = r16
SEND_BYTE:
    ldi r17,8
    mov r18,r16
_loop:
    rol r18
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
    ret


DELAY1:
    ldi r16,0
_loop:
    dec r16
    brne _loop
    ret

IMAGES:
;berry
.db 0x08, 0x10, 0x7c, 0xd6, 0xea, 0x7c, 0x38, 0x10
;seven
.db 0x7f, 0x63, 0x46, 0x0c, 0x18, 0x38, 0x38, 0x38

