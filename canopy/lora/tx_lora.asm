;chip atmega8



.include "d:\avr\avrasm\includes\m8def.inc"
.include "d:\tronix\avr\macros.inc.asm"

.equ F_OSC = 1000000
.equ UART_SPEED = 9600

.equ LORA_SS_PIN = PD5
.equ LORA_RESET_PIN = PC0
.equ MOSI = PB3
.equ MISO = PB4
.equ SCK = PB5


.equ LORA_MODE_SLEEP=0
.equ LORA_MODE_TX=0b011


.dseg
leds: .byte 1
sptr: .byte 1

.cseg
.org 0

rjmp RESET ; Reset Handler
reti;rjmp EXT_INT0 ; IRQ0 Handler
reti;rjmp EXT_INT1 ; IRQ1 Handler
reti;rjmp TIM2_COMP ; Timer2 Compare Handler
reti;rjmp TIM2_OVF ; Timer2 Overflow Handler
reti;rjmp TIM1_CAPT ; Timer1 Capture Handler
rjmp TIM1_COMPA ; Timer1 CompareA Handler
reti;rjmp TIM1_COMPB ; Timer1 CompareB Handler
reti;rjmp TIM1_OVF ; Timer1 Overflow Handler
reti;rjmp TIM0_OVF ; Timer0 Overflow Handler
reti;rjmp SPI_STC ; SPI Transfer Complete Handler
reti;rjmp USART_RXC ; USART RX Complete Handler
reti;rjmp USART_UDRE ; UDR Empty Handler
reti;rjmp USART_TXC ; USART TX Complete Handler
reti;rjmp ADC ; ADC Conversion Complete Handler
reti;rjmp EE_RDY ; EEPROM Ready Handler
reti;rjmp ANA_COMP ; Analog Comparator Handler
reti;rjmp TWSI ; Two-wire Serial Interface Handler
reti;rjmp SPM_RDY ; Store Program Memory Ready Handler


INIT:
	outi DDRC,0b00111101
	outi DDRD,(1 << LORA_SS_PIN) | (1 << PD1)

	;SPI init
	outi DDRB,(1 << PB2)|(1 << PB3)|(1 << PB5)
	; Enable SPI, Master, set clock rate fck/4
	outi SPCR,(1<<SPE) | (1<<MSTR)|(1<<SPR0)|(1<<SPR1)


	;UART init
	.equ UBR_VAL = F_OSC/(16*UART_SPEED)-1
	outiw UBRR,UBR_VAL
	outi UCSRB,(1 << TXEN) | (1 << RXEN)
	outi UCSRC,(1<<URSEL)|(1<<USBS)|(3<<UCSZ0)	;frame: 8 data 1 stop

	;Timer1 (16-bit) init
	;Clear on compare, CLK/256
	outi TCCR1B,(1 << WGM12) | (1 << CS12)

	.equ TIM1_DIV = (F_OSC/256)*5	;10 second
	outiw OCR1A,TIM1_DIV
	outi TIMSK,1 << OCIE1A

	rcall LORA_SS_OFF
	ret

PWR_ON:
	stsi leds,1
	ret

RESET:
	outiw SP,RAMEND
	sei
	ifpwron PWR_ON
	rcall INIT

	rcall LORA_RESET
	rcall LORA_INIT

	;ldi temp1,LORA_MODE_TX
	;rcall LORA_SET_MODE

	rjmp MAIN


MAIN:
	sbis UCSRA, RXC
	rjmp MAIN
	in temp,UDR
	;in temp1,UDR
	;sts leds,temp
	;rcall DISPLAY

	;to read LORA registers via UART
	rcall LORA_ADDR_R
	clr temp
	rcall SPI_TX
	rcall UART_TX
	rcall LORA_END_CMD

	rjmp MAIN



;#############################################
;IRQs
;#############################################

TIM1_COMPA:
	;reti
	pushSREG
	pushRegs

	rcall SHIFT_LEDS

	rcall LORA_TX_PACKET


	;ldi temp,'Q'
	;rcall UART_TX

	
	popRegs
	popSREG
	reti


;####################################   #########
;LORA
;#############################################

;#############################################

.macro conf
	ldi temp,@0
	ldi temp1,@1
	rcall LORA_SINGLE
.endm

LORA_INIT:

	;RegOpMode: mode = LoRa, LowFreqMode = 0n, SLEEP mode
	;conf 0x01,0b1001000
	ldi temp1,LORA_MODE_SLEEP
	rcall LORA_SET_MODE

	;RegPaConfig: RF pin = RFO, MaxPower = 15 dBm, OutputPower = 15 dBm
	conf 0x09,0b11111111	

	;RegOcp: Overload current protection ON, 100mA
	conf 0x0b,0b00101011

	;RegModemConfig1: bandwidth = 20.8 kHz, coding rate = 4/5, ImplicitHeaderMode = on
	;RegModemConfig2: SpreadingFactor = 12, ContinuousMode = off, RxCrcOn = false, SymbTimeout = not set
	;RegModemConfig3: LowDataRateOptimize = on, AGCAuto = ON
	conf 0x1D,0b00110011
	conf 0x1E,0b11000000
	conf 0x26,0b00001100

	;PreambleLength
	conf 0x20,0
	conf 0x21,0

	ret

LORA_SET_MODE:
	ori temp1,0b10000000
	ldi temp,0x01
	rcall LORA_SINGLE
	ret

LORA_RESET:
	cbi PORTC,LORA_RESET_PIN
	
	ldi temp,40
	clr temp1
	rcall DELAY

	sbi PORTC,LORA_RESET_PIN

	ldi temp,40
	clr temp1
	rcall DELAY

	ret

;temp - address
;temp1 - data
LORA_SINGLE:
	rcall LORA_ADDR_W
	mov temp,temp1
	rcall SPI_TX
	rcall LORA_END_CMD
	ret

LORA_ADDR_W:
	ori temp,0b10000000
	rcall LORA_CMD
	ret

LORA_ADDR_R:
LORA_CMD:
	rcall LORA_SS_ON
	rcall SPI_TX
	ret

LORA_END_CMD:
	rcall LORA_SS_OFF
	ret

;#############################################
LORA_SS_ON:
	nop
	nop
	cbi PORTD,LORA_SS_PIN
	nop
	nop
	ret

LORA_SS_OFF:
	nop
	nop
	sbi PORTD,LORA_SS_PIN
	nop
	nop
	ret

;#############################################
LORA_TX_PACKET:
	ldi temp,0
	rcall LORA_ADDR_W

	ldi temp,'Q'
	rcall SPI_TX
	ldi temp,'Q'
	rcall SPI_TX
	ldi temp,'Q'
	rcall SPI_TX
	ldi temp,'Q'
	rcall SPI_TX
	ldi temp,'Q'
	rcall SPI_TX
	ldi temp,'Q'
	rcall SPI_TX
	ldi temp,'Q'
	rcall SPI_TX
	ldi temp,'Q'
	rcall SPI_TX
	ldi temp,'Q'
	rcall SPI_TX
	ldi temp,'Q'
	rcall SPI_TX
	
	rcall LORA_END_CMD
	
	;RegPayloadLength
	conf 0x22,10

	ldi temp1,LORA_MODE_TX
	rcall LORA_SET_MODE
	ret

;#############################################
;CORE
;#############################################

SPI_TX:
	out SPDR,temp
SPI_TX_WAIT:
	sbis SPSR,SPIF
	rjmp SPI_TX_WAIT
	in temp,SPDR
	ret



UART_TX:
	sbis UCSRA,UDRE
	rjmp UART_TX
	out UDR,temp
	ret


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


;#############################################
SHIFT_LEDS:
	lds temp,leds
	clc
	sbrc temp,3
	sec
	rol temp
	sts leds,temp
	rcall DISPLAY
	ret

;#############################################
DISPLAY:
	lds temp,leds
	lsl temp
	lsl temp
	in temp1,PORTC
	andi temp1,0b11000011
	or temp1,temp
	out PORTC,temp1
	ret