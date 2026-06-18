#include <stdio.h>
#include "common.h"
#include "driver/spi_master.h"
#include "sonar_adc.h"
#include "sonar_tx.h"
#include "lcd.h"
#include "sd.h"
#include "gps.h"

#define TX_ENA	0
#define ADC_ENA 0


uint16_t buffer[ADC_RECORD_SAMPLES];

void init_spi2_host()
{
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
}



void app_main()
{
	printf("Entering app_main...\n");

	// init_spi2_host();
	sd_init();
	gps_init();

	// while(1)
	// {
	// 	gps_get_data();
	// 	printf("DATA OK\n");
	// 	vTaskDelay(pdMS_TO_TICKS(3000));
	// 	printf("DELAY OK\n");
	// }

	return;

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
				// sonar_uart_send_buffer(buffer, ADC_RECORD_SAMPLES);
				sd_save_ping(buffer, ADC_RECORD_SAMPLES);
				// break;
			}
			is_first = 0;
		}

		lcd_fill_screen(0,0,0);

		n++;

		// vTaskDelay(pdMS_TO_TICKS(BURST_TO_BURST_DELAY_MS));
		vTaskDelay(pdMS_TO_TICKS(1000));
		
	}
	
}

