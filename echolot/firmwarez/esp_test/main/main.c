#include <stdio.h>
#include "common.h"
#include "driver/spi_master.h"
#include "sonar.h"
#include "lcd.h"
#include "sd.h"
#include "gen.h"
#include "recorder.h"
#include "encoder.h"
#include "dsp.h"
#include "ui.h"
#include "uart_logger.h"
#include "esp_timer.h"


#define TX_ENA	0
#define ADC_ENA 1

#define NCYCLES	5


// uint16_t buffer[ADC_RECORD_SAMPLES];
uint16_t sonar_buffer[SONAR_BUF_SZ_SAMPLES];

void init_spi2_host()
{
	printf("Init SPI2 host...\n");
	// 1. Инициализация SPI шины с DMA
	spi_bus_config_t bus_cfg = {
		.mosi_io_num = MOSI_PIN,
		.miso_io_num = MISO_PIN,
		.sclk_io_num = SCLK_PIN,
		.quadwp_io_num = -1,
		.quadhd_io_num = -1,
		.max_transfer_sz = 25600, 
	};
	ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CH_AUTO));
	printf("SPI2 host init done!\n");
}

dev_status_t dstat = {
	.sd_ok = 0,
	.gps_ok = 0,
	.time_set = 0,
	.date_set = 0,
	.last_filenum = -1,
	.gps_updtime = 0,
	.gps_str = "--.----  --.---- S--",
	.lon = 0,
	.lat = 0,
	.satnum = 0,
	.datetime = "",
	.gps_enabled = 1,
	.ui_blocked = 0,
	.depth_set_mm = 1000
};
dev_status_t *DSTAT;

SemaphoreHandle_t spi_mutex;

// char info[1024];



void app_main()
{
	sonar_init();

	if(TX_DISABLE)
		printf("!!!!! WARNING !!!!! TX DISABLED (DCDC blocked)\n"); 

	printf("Entering app_main...\n");
	DSTAT = &dstat;

	// recorder_test();

	init_spi2_host();
	sd_init();
	

	ui_init();

	while(1)
	{
		vTaskDelay(pdMS_TO_TICKS(100));
	}

	
	
	
}

