#pragma once

#include "driver/gpio.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <inttypes.h>
#include "cons.h"
#include "lib.h"
#include "gps.h"
#include <time.h>
#include <sys/time.h>
#include "freertos/semphr.h"


#define TX_GPIO_1		13
#define TX_GPIO_2		14	//this used for IR2104
#define MOSDRV_ENA_PIN	26
#define DCDC_ENA_PIN	27

// SPI HOST2 params
#define MOSI_PIN	18
#define MISO_PIN	2
#define SCLK_PIN	19

// LCD params
#define LCD_ROWS		160
#define LCD_COLS		80
#define LCD_SPI_SPEED		1000000
#define LCD_LED_PIN		5
#define LCD_CS_PIN		23
#define LCD_DC_PIN		21
#define LCD_RST_PIN		22

// SD params
#define SD_CS_PIN		4

// GPS params
#define GPS_TX_PIN		33
#define GPS_RX_PIN		25
#define GPS_ENA_PIN		32

// Buttons
#define BUT1_PIN		17
#define BUT2_PIN		16


// TX params
#define TX_FREQ_HZ				187000
#define BURST_TO_BURST_DELAY_MS 40
#define IR_DSBL_DELAY_US		1000
#define MT_PRECHARGE_DELAY_MS	10

// ADC params
#define ADC_UNIT_USED           ADC_UNIT_1
#define ADC_CHANNEL_USED        ADC_CHANNEL_6     // GPIO34
#define ADC_SAMPLE_FREQ_HZ      250000

#define ADC_RECORD_TIME_MS		BURST_TO_BURST_DELAY_MS

#define ADC_RECORD_SAMPLES		(int)(ADC_RECORD_TIME_MS*ADC_SAMPLE_FREQ_HZ/1000)

#define UART_PORT               UART_NUM_0
#define UART_BAUDRATE           921600


typedef struct {
	uint8_t sd_ok;
	uint8_t time_set;
	uint8_t date_set;
	uint8_t gps_ok;
	time_t gps_updtime;
	float lon;
	float lat;
	uint8_t satnum;
	char gps_str[24];
	int16_t filenum;
	char datetime[64];
	gps_data_t *gps;
	uint8_t gps_enabled;

} dev_status_t;


extern dev_status_t *DSTAT;

extern SemaphoreHandle_t spi_mutex;
