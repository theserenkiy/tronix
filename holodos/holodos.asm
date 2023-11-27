;chip t2313
.include "C:/avr/includes/2313def.inc"

.equ FOSC = 10000000

.equ DELAY_TIMESTEP = DELAY_1ms

.equ PULSE_LEN_TRESHOLD = 15	;units: DELAY_TIMESTEP's

.equ DEBOUNCE_STEPS = 4			;number of sequential equal input states detected 
								;before output state will be switched

.equ IDLE_TIME_SEC = 20			;dead time after any output state been switched, 
								;(during that time input state detector not working)

.equ LED_PIN = 6
.equ RELAY_PIN = 0
.equ SIGNAL_PIN = 1


RESET:
	sbi DDRD,LED_PIN
	sbi DDRD,RELAY_PIN
	cbi DDRD,SIGNAL_PIN

	cbi PORTD,LED_PIN

MAIN:

	; sbi PORTD,6
	; rcall DELAY_1s
	; cbi PORTD,6
	; rcall DELAY_1s
	; rjmp MAIN

	ldi r20,IDLE_TIME_SEC
_wait_idle:
	rcall DELAY_1s
	dec r20
	brne _wait_idle


	ldi r22,0	;ON debounce counter
	ldi r23,0	;OFF debounce counter

_detect_loop:
	rcall CHECK_STATE

	cpi r22,DEBOUNCE_STEPS
	brsh _switch_on

	cpi r23,DEBOUNCE_STEPS
	brsh _switch_off

	;if nothing debounced - keep checking
	rjmp _detect_loop

_switch_on:
	sbi PORTD,LED_PIN
	sbi PORTD,RELAY_PIN
	rjmp _end

_switch_off:
	cbi PORTD,LED_PIN
	cbi PORTD,RELAY_PIN

_end:
	rjmp MAIN


;###################################
;checks current transmitting state (on/off) 
;and updates on/off debounce counters (r22/r23) 
;uses: r16-r19, r22,r23
CHECK_STATE:
	rcall GET_1st_PULSE_LENGTH

	cpi r16,PULSE_LEN_TRESHOLD
	brsh _set_off

_set_on:
	ldi r23,0
	inc r22
	sbic PORTD,LED_PIN
	rjmp _end

	sbi PORTD,LED_PIN
	rcall DELAY_10ms
	cbi PORTD,LED_PIN
	rjmp _end

_set_off:
	ldi r22,0
	inc r23
	sbis PORTD,LED_PIN
	rjmp _end

	cbi PORTD,LED_PIN
	rcall DELAY_10ms
	rcall DELAY_10ms
	sbi PORTD,LED_PIN


_end:
	ret

;###################################
;get duration of 1st '0' pulse after long '1' (above 1s)  
;duration units = DELAY_TIMESTEP time
;uses r16,r17,r18,r19
;returns r16 - time in ms
GET_1st_PULSE_LENGTH:
	rcall WAIT_ONE_KEEPS_1s

_wait_zero:
	sbic PIND,1
	rjmp _wait_zero

	ldi r19,0	;counter

_while_zero:
	rcall DELAY_TIMESTEP
	cpi r19,0xff
	breq _skip_count
	inc r19
_skip_count:
	sbis PIND,1
	rjmp _while_zero
	mov r16,r19
	ret


;###################################
;returns after PD1 keeps '1' for 1 sec
;uses r16,r17,r18,r19
WAIT_ONE_KEEPS_1s:
_start:
	ldi r19,4
_loop0:	
	ldi r18,250
_loop1:
	rcall DELAY_1ms
	sbis PIND,1
	rjmp _start
	dec r18
	brne _loop1
	dec r19
	brne _loop0
	ret


;###################################
;uses r16,r17
DELAY_1ms:
	ldi r16,0
	ldi r17,FOSC/768000
_loop:
	dec r16
	brne _loop
	dec r17
	brne _loop
	ret
;###################################
;uses r16,r17,r18
;DELAY_TIMESTEP:
DELAY_10ms:
	ldi r18,10
_loop:
	rcall DELAY_1ms
	dec r18
	brne _loop
	ret

;###################################
;uses r16,r17,r18,r19
DELAY_1s:
	ldi r19,4
_loop0:
	ldi r18,250
_loop1:
	rcall DELAY_1ms
	dec r18
	brne _loop1
	dec r19
	brne _loop0
	ret
