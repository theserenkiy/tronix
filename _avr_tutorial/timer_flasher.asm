;chip m48
.include "m48def.inc"

.cseg
.org 0
    rjmp RESET
.org 0x00B
    rjmp IRQ_TIM1_COMPA


RESET:
	rcall  TIMER1_INIT
    sbi DDRB,0
    sei                     

MAIN:
  
	rjmp MAIN



TIMER1_INIT:
    ldi r16, 0b00000000
    sts TCCR1A, r16
    ldi r16, 0b00001011
    sts TCCR1B, r16
    ldi r16, 0b00000000
    sts TCCR1C, r16
    ldi r16, 0b00000010
    sts TIMSK1, r16

    ldi r16, high(31250)	
    sts OCR1AH, r16
    ldi r16, low(31250)
    sts OCR1AL, r16

    ret

IRQ_TIM1_COMPA:
    sbic PORTB,0    
    rjmp _off       
    sbi PORTB,0     
    rjmp _end       
_off:
    cbi PORTB,0
_end:
    reti
