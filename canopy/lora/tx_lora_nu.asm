;chip atmega8

.include "d:\avr\avrasm\includes\m8def.inc"
.include "d:\tronix\avr\macros.inc.asm"


.dseg
leds: .byte 1
sptr: .byte 1


.cseg
.org 0


rcall RESET ; Reset Handler
reti;rcall EXT_INT0 ; IRQ0 Handler
reti;rcall EXT_INT1 ; IRQ1 Handler
reti;rcall TIM2_COMP ; Timer2 Compare Handler
reti;rcall TIM2_OVF ; Timer2 Overflow Handler
reti;rcall TIM1_CAPT ; Timer1 Capture Handler
reti;rcall TIM1_COMPA ; Timer1 CompareA Handler
reti;rcall TIM1_COMPB ; Timer1 CompareB Handler
reti;rcall TIM1_OVF ; Timer1 Overflow Handler
reti;rcall TIM0_OVF ; Timer0 Overflow Handler
reti;rcall SPI_STC ; SPI Transfer Complete Handler
reti;rcall USART_RXC ; USART RX Complete Handler
reti;rcall USART_UDRE ; UDR Empty Handler
reti;rcall USART_TXC ; USART TX Complete Handler
reti;rcall ADC ; ADC Conversion Complete Handler
reti;rcall EE_RDY ; EEPROM Ready Handler
reti;rcall ANA_COMP ; Analog Comparator Handler
reti;rcall TWSI ; Two-wire Serial Interface Handler
reti;rcall SPM_RDY ; Store Program Memory Ready Handler



INIT:
	outi DDRC,0b00111100
	outi PORTC,0b00111100
	stsi sptr,0
	ret

PWR_ON:
	stsi leds,1
	ret

RESET:
	rcall INIT
	ifpwron PWR_ON
	;rcall SHIFT_LEDS
	rjmp MAIN


MAIN:
	ldi temp,1
	clr temp1
	rcall DELAY

	lds temp,sptr
	rdpm NU,temp
	inc temp
	sts sptr,temp
	mov temp,r0
	tst temp
	breq RESET_STR_PTR
	sts leds,temp
	rcall DISPLAY
	rjmp MAIN


RESET_STR_PTR:
	stsi sptr,0
	rjmp MAIN

SHIFT_LEDS:
	lds temp,leds
	clc
	sbrc temp,3
	sec
	rol temp
	sts leds,temp
	rcall DISPLAY
	ret

DISPLAY:
	lds temp,leds
	lsl temp
	lsl temp
	out DDRC,temp
	ret


NU:
;.db 0b00000001,0b00000010,0b00000100,0b00001000,0
.db 0b00001111,0,0,0b00000100,0b00000100,0b00000100,0b00000100,0b00001111,16,16,16,16
.db 0b00001111,0b00000100,0b00000100,0b00000100,0b00000110,0b00001001,0b00001001,0b00001001,0b00001001,0b00000110,16,16,16,16,16,16,16,16,0;

;.db 0b1001,0b1111,0b1001,16,16,16,16,16
;.db 0b0110,0b1110,0b1110,0b0111,0b1110,0b1110,0b0110,16,16,16,16,16
;.db 0b1000,0b1000,0b1111,0b1000,0b1000,16,16,16
;.db 0b0111,0b1010,0b1010,0b0111,16,16
;.db 0b0110,0b1001,0b1001,0b1001,16,16
;.db 0b1111,0b0101,0b0101,0b0111,16,16,16,16,16,16,16,16,16,16,16,16,0

;#############################################
;temp - high byte
;temp1 - low byte
;cycles: (temp1*3-1)*(temp2*4) = 12*temp2*temp3; max = 768 * 1024 = 786432
DELAY:
	;ret
	mov temp2,temp1
DELAY_LOOP:	
	dec temp2
	brne DELAY_LOOP
	dec temp
	brne DELAY
	ret