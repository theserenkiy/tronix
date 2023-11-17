;chip t13
.include "c:/avr/includes/tn13def.inc"

.equ WDOG_CFG = 0 << WDP3 | 1 << WDP2 | 1 << WDP1 | 0 << WDP0 | 1 << WDTIE


.include "heart.lib.asm"

RESET:
	rcall COMMON_SETTINGS
	sei
	rjmp MAIN


MAIN:
	rcall RX_CYCLE
	;rcall TEST_CYCLE
	rjmp MAIN


RX_CYCLE:
	;timer 40
	rcall RX_NOISE_REDUCTED
	;timer_off
	sbrc r17,0
	rcall RCVD_1
	;sleep
	ret


RCVD_1:
	;wait 0
	;rcall TX_PULSE
	rcall BEAT
	ret



;##############################################################################
; INTERRUPTS

IRQ_WDOG:
	reti




;##############################################################################
; FUNCTIONS
