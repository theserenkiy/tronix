#include <stdio.h>
#include "common.h"
#include "driver/spi_master.h"
#include "sonar_adc.h"
#include "sonar_tx.h"
#include "lcd2.h"
#include "sd.h"
#include "gps.h"
#include "uart_logger.h"
#include <time.h>
#include <sys/time.h>

#define TX_ENA	0
#define ADC_ENA 1


uint16_t buffer[ADC_RECORD_SAMPLES];

void init_spi2_host()
{
	printf("Init SPI2 host...\n");
	// 1. Инициализация SPI шины с DMA
	spi_bus_config_t bus_cfg = {
		.mosi_io_num = MOSI_PIN,
		.miso_io_num = MISO_PIN,  // ST7735S в режиме SPI использует только MOSI
		.sclk_io_num = SCLK_PIN,
		.quadwp_io_num = -1,
		.quadhd_io_num = -1,
		.max_transfer_sz = 24000,  // Макс. размер для DMA
	};
	ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CH_AUTO));
	printf("SPI2 host init done!\n");
}

dev_status_t dstat = {
	.sd_ok = 0,
	.gps_ok = 0,
	.time_set = 0,
	.date_set = 0,
	.filenum = -1,
	.lon = 0,
	.lat = 0,
	.time = ""
};
dev_status_t *DEVSTATUS;

void status_task(void *prm)
{
	while(1)
	{
		time_t now;
		struct tm timeinfo;
		time(&now);
		localtime_r(&now, &timeinfo);

		// Time update
		strftime(DEVSTATUS->time, sizeof(DEVSTATUS->time), "%H:%M:%S", &timeinfo);

		// SD card
		if(!sd_check())
		{
			sd_init();
			DEVSTATUS->sd_ok = sd_check();
		}

		
		gps_read();
		char info[1024];
		gps_info(info);
		printf("GPS INFO: \n%s\n",info);


		lcd2_update();
		delay_ms(1000);
	}
}

void app_main()
{
	printf("Entering app_main...\n");

	DEVSTATUS = &dstat;

	init_spi2_host();
	sd_init();
	gps_init();

	lcd2_init();

	xTaskCreate(
		status_task,    // Pointer to the task function
		"STATUS_TASK",    // Debug name string (Max 16 chars)
		4096,              // Stack size in BYTES (Note: Vanilla FreeRTOS uses words, ESP32 uses bytes)
		NULL,              // Pointer to pass parameters (NULL if none)
		1,                 // Task priority (Higher number = Higher priority)
		NULL              // Task handle pointer (NULL if not needed)
	);
	

	if(TX_ENA)
	{
		sonar_tx_init();
	}
	else
	{
		gpio_reset_pin(MOSDRV_ENA_PIN);
		gpio_set_direction(MOSDRV_ENA_PIN, GPIO_MODE_OUTPUT);
		gpio_set_level(MOSDRV_ENA_PIN, 1);

		gpio_reset_pin(TX_GPIO_2);
		gpio_set_direction(TX_GPIO_2, GPIO_MODE_OUTPUT);
		gpio_set_level(TX_GPIO_2, 0);
	}
	
	if(ADC_ENA)
		sonar_adc_init();
	
	// lcd_init();

	vTaskDelay(pdMS_TO_TICKS(1000));

	int is_first = 1;
	int n = 0;
	while(1)
	{
		if(TX_ENA)
			sonar_tx_burst(32, 1);
		
		
		if(ADC_ENA)
		{
			if(is_first)
			{
				sonar_adc_capture(buffer, ADC_RECORD_SAMPLES);
				// uart_logger_send_buffer(buffer, ADC_RECORD_SAMPLES);
				sd_save_ping(buffer, ADC_RECORD_SAMPLES);
				// break;
			}
			is_first = 0;
		}

		// lcd_fill_screen(0,0,0);

		n++;

		// vTaskDelay(pdMS_TO_TICKS(BURST_TO_BURST_DELAY_MS));
		vTaskDelay(pdMS_TO_TICKS(1000));
		
	}
	
}

