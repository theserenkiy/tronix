.macro outi
	ldi r25,@1
	out @0,r25
.endm

.macro acomp_on	;<use_interrupts>
	;sbi PORTB,PIN_DIVIDER
	cbi DDRB,PIN_AREF
	cbi DDRB,PIN_RF
	outi ACSR,(@0 << ACIE | 1 << ACIS1 | 1 << ACIS0)
	outi DIDR0,0b11
.endm

.macro acomp_off
	outi ACSR,1 << ACD
	outi DIDR0,0
.endm

.macro drain_cap
	cbi PORTB,PIN_RF
	sbi DDRB,PIN_RF
	nop
	nop
	nop
	cbi DDRB,PIN_RF
.endm

.macro wait
	ldi r16,@0
	rcall DELAY
.endm

.macro timer
	ldi r16,@0
	rcall START_TIMER
.endm

.macro timer_off
	rcall STOP_TIMER
.endm
