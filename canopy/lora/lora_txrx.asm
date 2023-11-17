;chip atmega8


.include "d:\avr\avrasm\includes\m8def.inc"
.include "d:\tronix\avr\macros.inc.asm"

.equ F_OSC = 1000000
.equ BEEP_OSC = F_OSC/64
.equ UART_SPEED = 9600

.equ LORA_SS_PIN = PD5
.equ LORA_RESET_PIN = PC0
.equ MOSI = PB3
.equ MISO = PB4
.equ SCK = PB5

.equ LED0 = PC5
.equ LED1 = PC4
.equ LED2 = PC3
.equ LED3 = PC2

.equ LORA_IRQ_TX_DONE=3
.equ LORA_IRQ_RX_DONE=6

.equ MODES_COUNT = 2
.equ MODE_RX = 0
.equ MODE_TX = 1

.equ STATE_WAIT=0
.equ STATE_TX=1
.equ STATE_RX=2

.equ PAYLOAD_LENGTH=15

.macro blink_led
	sbi PORTC,@0
	clr temp
	rcall DELAY
	cbi PORTC,@0
.endm

.macro toggle_led
	lds temp,leds
	eori temp,1 << @0
	sts leds,temp
	rcall DISPLAY
.endm

.macro set_led
	sbm leds,@0
	rcall DISPLAY
.endm
.macro clr_led
	cbm leds,@0
	rcall DISPLAY
.endm
.macro beep
	rcall BEEP_INIT
	outiw OCR1A,(BEEP_OSC/@0/2)
	stsi beep_counter,@1
.endm

.macro dbgr
	in temp,@0
	rcall UART_TX
.endm

.dseg

leds: .byte 1
mode: .byte 1
state: .byte 1
counter: .byte 1
beep_counter: .byte 1
modem_status: .byte 1
str: .byte 20
.cseg
.org 0

rjmp RESET ; Reset Handler
reti;rjmp EXT_INT0 ; IRQ0 Handler
reti;rjmp EXT_INT1 ; IRQ1 Handler
rjmp TIM2_COMP ; Timer2 Compare Handler
reti;rjmp TIM2_OVF ; Timer2 Overflow Handler
reti;rjmp TIM1_CAPT ; Timer1 Capture Handler
reti;	rjmp TIM1_COMPA ; Timer1 CompareA Handler
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

.include "./lora_iface.inc.asm"
.include "./core.inc.asm"

FREQS:
;band 433
.db 0x6c, 0x44, 0xcc, 0 ;0: 433.075
.db 0x6c, 0x51, 0x99, 0 ;1: 433.275
.db 0x6c, 0x5e, 0x66, 0 ;2: 433.475
.db 0x6c, 0x6b, 0x33, 0 ;3: 433.675
.db 0x6c, 0x78, 0x00, 0 ;4: 433.875
.db 0x6c, 0x84, 0xcc, 0 ;5: 434.075
.db 0x6c, 0x91, 0x99, 0 ;6: 434.275
.db 0x6c, 0x9e, 0x66, 0 ;7: 434.475

;band 446
.db 0x6f, 0x81, 0x33, 0 ;0: 446.01875
.db 0x6f, 0x82, 0xcc, 0 ;1: 446.04375
.db 0x6f, 0x84, 0x66, 0 ;2: 446.06875
.db 0x6f, 0x87, 0x99, 0 ;3: 446.11875
.db 0x6f, 0x89, 0x33, 0 ;4: 446.14375
.db 0x6f, 0x8a, 0xcc, 0 ;5: 446.16875
.db 0x6f, 0x8c, 0x66, 0 ;6: 446.19375


NOTES:	.db 0,BEEP_OSC/523,BEEP_OSC/587,BEEP_OSC/659,BEEP_OSC/698,BEEP_OSC/783,BEEP_OSC/880,BEEP_OSC/987


TX_STR:
;.db "Privet, Piter!!!",0;
PARTITURE:
.db 3,5,6,5,7,6,5,6,3,3,5,6,3,3,3,0;

INIT:
	outi DDRC,0b00111101
	outi DDRD,(1 << LORA_SS_PIN) | (1 << PD1)

	;SPI init
	outi DDRB,(1 << PB2)|(1 << PB3)|(1 << PB5)|(1 << PB1)
	; Enable SPI, Master, set clock rate fck/4
	outi SPCR,(1<<SPE) | (1<<MSTR)|(1<<SPR0)|(1<<SPR1)


	;UART init
	.equ UBR_VAL = F_OSC/(16*UART_SPEED)-1
	outiw UBRR,UBR_VAL
	outi UCSRB,(1 << TXEN) | (1 << RXEN)
	outi UCSRC,(1<<URSEL)|(1<<USBS)|(3<<UCSZ0)	;frame: 8 data 1 stop

	;Timer1 (16-bit) init
	;Toggle OC1A on compare
	;outi TCCR1A,(1 << COM1A0)
	;Clear on compare, CLK/64
	outi TCCR1B,(1 << WGM12) | (1 << CS11) | (1 << CS10)

	;outiw OCR1A,(BEEP_OSC/400/2)
	outi OCR1AH,0
	outi OCR1AL,(BEEP_OSC/400/2)


	;Timer2 generates interrupts every 10ms - for delays and other timing stuff
	outi TCCR2,(1 << WGM21) | (1 << CS22) | (1 << CS21) | (1 << CS20)
	outi OCR2,(F_OSC/1024/100)

	outi TIMSK,(1 << OCIE2)

	rcall LORA_SS_OFF
	stsi state,0
	ret

PWR_ON:
	stsi mode,MODE_RX
	stsi counter,0
	stsi beep_counter,0
	ret

RESET:
	outiw SP,RAMEND

	lds temp,mode
	cinc temp,MODES_COUNT
	sts mode,temp

	ifpwron PWR_ON

	rcall INIT

	sei

	beep 3000,20

	rcall LORA_RESET
	rcall LORA_INIT

	rcall SHOW_MODE

	rcall GET_MODE_VECTOR
	vector MODES,temp

	rcall LOAD_PARTITURE
	;rcall PLAY_MUSIC

	rjmp MAIN

MAIN:
	;rjmp MAIN
	;rcall CHECK_TIMEOUT

	rcall GET_MODE_VECTOR
	inc temp
	vector MODES,temp 	;call %MODE%_CYCLE

	rjmp MAIN


;#############################################
;#############################################
;#          ----    IRQs    ----             #
;#############################################
;#############################################

TIM2_COMP:
	pushSREG
	pushRegs

	rcall EVERY_10ms
	lds temp,counter
	cinc temp,100
	sts counter,temp

	lds temp,beep_counter
	tst temp
	breq END_T2C
	;rcall UART_TX
	dec temp
	sts beep_counter,temp
	brne END_T2C
	rcall BEEP_END

END_T2C:
	popRegs
	popSREG
	reti

;#############################################
;SYSTEM
;#############################################

SHOW_MODE:
	ldi temp,1	;value for shift
	lds temp1,mode
	rcall SHIFT
	sts leds,temp
	rcall DISPLAY_L2R

	ldi temp,50
	rcall DELAY_x10ms

	stsi leds,0
	rcall DISPLAY
	ret

;#############################################
GET_MODE_VECTOR:

	lds temp,mode
	lsl temp
	lsl temp
	ret


MODES:
	rjmp MODE_RX_INIT
	rjmp MODE_RX_CYCLE
	rjmp MODE_RX_BUTTON
	nop

	rjmp MODE_TX_INIT
	rjmp MODE_TX_CYCLE
	rjmp MODE_TX_BUTTON
	nop

;#############################################
MODE_RX_INIT:

	ldi temp,LORA_MODE_RX
	rcall LORA_SET_MODE
	ret

MODE_RX_CYCLE:

	lora_read 0x18 	;ModemStatus
	lds temp1,modem_status
	cp temp,temp1
	breq MDRX_CHECK_IRQ

	sts modem_status,temp
	clr temp1
	sbrc temp,0	;signal detected
	sbr temp1,1 << 1
	sbrc temp,1	;signal synchronized
	sbr temp1,1 << 2
	;sbrc temp,3	;header info valid
	;sbr temp1,1 << 3
	tst temp1
	breq MDRX_SET_LEDS
	sbrc temp1,2
	rjmp MDRX_BEEP1
	beep 600,20
	rjmp MDRX_SET_LEDS

MDRX_BEEP1:
	beep 1000,20
	rjmp MDRX_SET_LEDS

MDRX_SET_LEDS:
	sts leds,temp1
	rcall DISPLAY

MDRX_CHECK_IRQ:
	rcall LORA_READ_IRQ
	sbrc temp,LORA_IRQ_RX_DONE
	rjmp MODE_RX_READ_PACKET
	ldi temp,20
	rcall DELAY_x10ms
	ret

MODE_RX_READ_PACKET:
	;rcall UART_TX
	set_led 3
	;beep 1500,50
	ldi temp,20
	rcall DELAY_x10ms

	lora_read 0x10	;last packet FIFO address
	;rcall UART_TX
	mov temp1,temp
	lora_writereg 0x0D	;FIFO pointer

	ldi temp,0
	rcall LORA_CMD

	ldi temp1,PAYLOAD_LENGTH
MODE_RX_READ_BYTE:
	rcall SPI_TX
	rcall UART_TX
	rcall PLAY_NOTE
	dec temp1
	brne MODE_RX_READ_BYTE

	rcall LORA_END_CMD

	clr_led 3
	ret

MODE_RX_BUTTON:
	ret
;#############################################
MODE_TX_INIT:
	;ldi temp,LORA_MODE_TX
	;rcall LORA_SET_MODE
	ret

MODE_TX_CYCLE:
	;toggle_led 2
	lds temp,state
	tst temp
	breq MODE_TX_SEND_PACKET
	rcall LORA_READ_IRQ
	sbrs temp,LORA_IRQ_TX_DONE
	ret

	clr_led 3
	ldi temp,150
	rcall DELAY_x10ms
	rjmp MODE_TX_SEND_PACKET

MODE_TX_SEND_PACKET:
	stsi state,STATE_TX
	set_led 3
	ldiw X,str
	rcall LORA_TX_PACKET
	ret

MODE_TX_BUTTON:
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

DISPLAY_R2L:
	lds temp1,leds
	clr temp
	ror temp1
	rol temp
	ror temp1
	rol temp
	ror temp1
	rol temp
	ror temp1
	rol temp
	rjmp DISPLAY_OUT

DISPLAY:
DISPLAY_L2R:
	lds temp,leds
DISPLAY_OUT:
	lsl temp
	lsl temp
	in temp1,PORTC
	andi temp1,0b11000011
	or temp1,temp
	out PORTC,temp1
	ret


;#############################################

BEEP_INIT:
	outi TCCR1A,(1 << COM1A0)
	outiw TCNT1,0
	ret

;#############################################
BEEP_END:
	outi TCCR1A,0
	ret

;#############################################
;loading partiture to SRAM
LOAD_PARTITURE:
	ldi temp1,0
	ldiw X,str
READ_PARTITURE_BYTE:
	rdpm PARTITURE,temp1
	inc temp1
	mov temp,r0
	tst temp
	breq END_LOAD_PARTITURE
	rdpm NOTES,temp
	mov temp,r0
	lsr temp
	st X+,temp
	rcall UART_TX
	rjmp READ_PARTITURE_BYTE

END_LOAD_PARTITURE:
	ldi temp,0
	st X+,temp
	ret

;#############################################
PLAY_MUSIC:
	ldiw X,str
LOAD_NOTE:
	ld temp,X+
	tst temp
	breq PLAY_MUSIC_END
	rcall PLAY_NOTE
	rjmp LOAD_NOTE
PLAY_MUSIC_END:
	ret

;
PLAY_NOTE:
	outi OCR1AH,0
	out OCR1AL,temp
	rcall BEEP_INIT
	ldi temp,10
	rcall DELAY_x10ms
	rcall BEEP_END
	ret
