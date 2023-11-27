;#############################################
;LORA
;#############################################

.equ LORA_MODE_SLEEP=0
.equ LORA_MODE_TX=0b011
.equ LORA_MODE_RX=0b101

.macro lora_write
	ldi temp1,@1
	lora_writereg @0
.endm
.macro lora_writereg
	ldi temp,@0
	rcall LORA_SINGLE_W
.endm
.macro lora_read
	ldi temp,@0
	ldi temp1,0
	rcall LORA_SINGLE
	tst temp
.endm

;#############################################
LORA_INIT:

	;write 0x01,0b1001000
	ldi temp,LORA_MODE_SLEEP
	rcall LORA_SET_MODE

	;RegPaConfig: RF pin = RFO, MaxPower = 15 dBm, OutputPower = 15 dBm
	lora_write 0x09,0b11111111	

	;RegOcp: Overload current protection ON, 240mA
	lora_write 0x0b,0b00111011

	;RegModemConfig1: bandwidth = 20.8 kHz, coding rate = 4/5, ImplicitHeaderMode = on
	lora_write 0x1D,0b00110011
	
	;RegModemConfig1: bandwidth = 10.4 kHz, coding rate = 4/5, ImplicitHeaderMode = on
	;lora_write 0x1D,0b00010011

	;RegModemConfig2: SpreadingFactor = 12, ContinuousMode = off, RxCrcOn = false, SymbTimeout = not set
	lora_write 0x1E,0b11000000
	;RegModemConfig3: LowDataRateOptimize = on, AGCAuto = ON
	lora_write 0x26,0b00001100

	;PreambleLength
	lora_write 0x20,0
	lora_write 0x21,8

	;Payload length
	lora_write 0x22,PAYLOAD_LENGTH

	ret

;#############################################
LORA_SET_MODE:
	mov temp1,temp
	;RegOpMode: mode = LoRa, LowFreqMode = On
	ori temp1,0b10000000
	ldi temp,0x01
	rcall LORA_SINGLE_W
	ret

;#############################################
LORA_READ_IRQ:
	lora_read 0x12
	brne LORA_CLEAR_IRQ
	ret
LORA_CLEAR_IRQ:	
	push temp
	lora_write 0x12,0xff
	pop temp
	ret

;#############################################
LORA_RESET:
	cbi PORTC,LORA_RESET_PIN
	
	ldi temp,2
	rcall DELAY_x10ms

	sbi PORTC,LORA_RESET_PIN

	ldi temp,2
	rcall DELAY_x10ms

	ret

;#############################################
;temp - address
;temp1 - data
LORA_SINGLE_W:
	ori temp,0b10000000
LORA_SINGLE:
	rcall LORA_CMD
	mov temp,temp1
	rcall SPI_TX
	;rcall UART_TX
	rcall LORA_END_CMD
	ret

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
	lora_write 0x0D,0x80	;FIFO ptr
	ldi temp,0b10000000
	rcall LORA_CMD
	clr temp1	;bytes counter

LORA_TX_BYTE:
	ld temp,X+
	rcall SPI_TX
	inc temp1
	tst temp
	brne LORA_TX_BYTE

	rcall LORA_END_CMD
	
	;0x22 - RegPayloadLength
	;lora_write temp1 as data length
	lora_write 0x22,PAYLOAD_LENGTH

	ldi temp,LORA_MODE_TX
	rcall LORA_SET_MODE
	ret