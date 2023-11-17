;chip t13
.include "c:/avr/includes/tn13def.inc"

.equ WDOG_CFG = 0 << WDP3 | 0 << WDP2 | 1 << WDP1 | 1 << WDP0 | 1 << WDTIE

.include "heart.headers.asm"


.org 0x000A
.include "heart.lib.asm"
RESET:
	rcall COMMON_SETTINGS
	sbi PORTB,PIN_RF
	;rjmp DEADLOOP
	cbi PORTB,PIN_RF
	rjmp MAIN


MAIN:
	rcall TX_PULSE
	;rcall BEAT
	;rcall READ_RESPONSE
	sleep
	rjmp MAIN

READ_RESPONSE:
	wait 100
	timer 40
	rcall RX_NOISE_REDUCTED
	timer_off
	sbrc r17,0
	rcall RCVD
	ret

RCVD:
	rcall BEAT
	ret

;##############################################################################
; INTERRUPTS

IRQ_WDOG:
	reti



;##############################################################################
; FUNCTIONS
