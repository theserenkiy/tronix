#pragma once

#include "driver/gpio.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include <inttypes.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>



#define TX_DISABLE		0	// Blocks DCDC from working
#define RECORDER_NO_WRITE	0

#define SONAR_BUF_MAX_SZ_BYTES	120*1024


#define TX_GPIO_1		13
#define TX_GPIO_2		14	//this used for IR2104
#define TX_TEST_PIN		5
#define MOSDRV_ENA_PIN	26
#define DCDC_ENA_PIN	27

// SPI HOST2 params
// #define MOSI_PIN	18
// #define MISO_PIN	2
// #define SCLK_PIN	19

#define MOSI_PIN	23
#define MISO_PIN	19
#define SCLK_PIN	18


// LCD params
#define LCD_ROWS		160
#define LCD_COLS		80
#define LCD_SPI_SPEED		1000000
#define LCD_LED_PIN		2
#define LCD_CS_PIN		15
#define LCD_DC_PIN		21
#define LCD_RST_PIN		22

// SD params
#define SD_CS_PIN		4

// GPS params
#define GPS_TX_PIN		33
#define GPS_RX_PIN		25
#define GPS_ENA_PIN		32

// Buttons
#define ENC_A_PIN		17
#define ENC_B_PIN		16
#define BUT0_PIN		5
#define BUT1_PIN		0



// TX params
#define IR_DSBL_DELAY_US		1000
#define MT_PRECHARGE_DELAY_MS	10

// ADC params
#define ADC_UNIT_USED           ADC_UNIT_1
#define ADC_CHANNEL_USED        ADC_CHANNEL_6     // GPIO34
#define ADC_SAMPLE_FREQ_HZ      500000
#define ADC_REAL_FS_HZ			125000

#define ADC_RECORD_TIME_MS		80
#define ADC_RECORD_SAMPLES		(int)(ADC_RECORD_TIME_MS*ADC_SAMPLE_FREQ_HZ/1000)


#define SONAR_BUF_SZ_PINGS		(int)(SONAR_BUF_MAX_SZ_BYTES/2/ADC_RECORD_SAMPLES)
#define SONAR_BUF_SZ_SAMPLES	ADC_RECORD_SAMPLES * SONAR_BUF_SZ_PINGS

#define WAV_INFO_SZ				2048


#define UART_PORT               UART_NUM_0
#define UART_BAUDRATE           921600

#define TESTCONFIG_PATH		"/sdcard/_testconfig.txt"


typedef struct {
	uint8_t sd_ok;
	int16_t last_filenum;
	char last_filename[32];
	// gps_data_t *gps;
	uint8_t gps_enabled;
	float depth_set_mm;
	uint8_t ui_blocked;
	uint16_t center_freq_khz;
	uint16_t testcfg_idx;
	

} dev_status_t;

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  ((byte) & 0x80 ? '1' : '0'), \
  ((byte) & 0x40 ? '1' : '0'), \
  ((byte) & 0x20 ? '1' : '0'), \
  ((byte) & 0x10 ? '1' : '0'), \
  ((byte) & 0x08 ? '1' : '0'), \
  ((byte) & 0x04 ? '1' : '0'), \
  ((byte) & 0x02 ? '1' : '0'), \
  ((byte) & 0x01 ? '1' : '0')


extern uint16_t sonar_buffer[SONAR_BUF_SZ_SAMPLES];

extern dev_status_t *DSTAT;

extern SemaphoreHandle_t spi_mutex;


#include "cons.h"
#include "lib.h"
#include "ui.h"
#include "lcd.h"
#include "dsp.h"
#include "wav.h"
#include "recorder.h"
#include "confparser.h"