#include <stdio.h>
#include "common.h"
#include "driver/spi_master.h"
#include "sonar.h"
#include "lcd2.h"
#include "sd.h"
#include "gps.h"
#include "chirp.h"
#include "uart_logger.h"
#include "esp_timer.h"


#define TX_ENA	0
#define ADC_ENA 1

#define NCYCLES	5



// uint16_t buffer[ADC_RECORD_SAMPLES];
uint16_t big_buffer[ADC_RECORD_SAMPLES*6];

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
		.max_transfer_sz = 24000, 
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
	.gps_updtime = 0,
	.gps_str = "--.----  --.---- S--",
	.lon = 0,
	.lat = 0,
	.satnum = 0,
	.datetime = "",
	.gps_enabled = 1
};
dev_status_t *DSTAT;

SemaphoreHandle_t spi_mutex = NULL;

char info[1024];

void status_task(void *prm)
{
	
	while(1)
	{
		time_t now;
		struct tm timeinfo;
		time(&now);
		localtime_r(&now, &timeinfo);

		// Time update
		strftime(DSTAT->datetime, sizeof(DSTAT->datetime), "%Y.%m.%d %H:%M:%S", &timeinfo);

		// SD card
		if(!sd_check())
		{
			sd_init();
			DSTAT->sd_ok = sd_check();
		}

		
		DSTAT->gps_ok = DSTAT->gps_updtime && DSTAT->gps_updtime > now-30;

		// gps_info(info);
		// printf("GPS INFO: \n%s\n",info);


		lcd2_update();
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}

void make_measure()
{
	FILE *fp = sd_open_wav(ADC_RECORD_SAMPLES*NCYCLES*6);

	// lcd2_sleep(1);
	// lcd2_sleep(1);
	gps_enable(0);
	sonar_precharge(500);
	for(int cyc=0; cyc < 1; cyc++)
	{
		for(int i=0; i < 3; i++)
		{
			// sonar_tx_burst(32, 1);
			sonar_ping();
			sonar_adc_capture(big_buffer+(ADC_RECORD_SAMPLES *i), ADC_RECORD_SAMPLES);
			delay_ms(BURST_TO_BURST_DELAY_MS);
			// uart_logger_send_buffer(buffer, ADC_RECORD_SAMPLES);
		}

		for(int i=3; i < 6; i++)
		{
			// sonar_tx_burst(32, 1);
			sonar_ping2();
			sonar_adc_capture(big_buffer+(ADC_RECORD_SAMPLES *i), ADC_RECORD_SAMPLES);
			delay_ms(BURST_TO_BURST_DELAY_MS);
			// uart_logger_send_buffer(buffer, ADC_RECORD_SAMPLES);
		}
		fwrite(big_buffer,2,ADC_RECORD_SAMPLES*6,fp);
	}
	fclose(fp);
	printf("File closed\n");
	// sd_save_ping(big_buffer, ADC_RECORD_SAMPLES*6);
	// lcd2_sleep(0);
	// lcd2_waveform(big_buffer, ADC_RECORD_SAMPLES, 0);
	sonar_charge(1);
	// gps_enable(1);
}

void button_pressed(int num)
{
	printf("BUTTON PRESSED %d\n",num);
	if(num==0)
	{
		make_measure();	
	}
	else if(num==1)
	{
		lcd2_waveform(big_buffer, ADC_RECORD_SAMPLES, 1);
		
	}
}

void app_main()
{
	printf("Entering app_main...\n");
	DSTAT = &dstat;

	init_spi2_host();
	sd_init();

	// sd_speed_test();
	
	spi_mutex = xSemaphoreCreateMutex();

	lcd2_init();
	sonar_tx_init();
	sonar_adc_init();
	// sonar_charge(1);



	xTaskCreate(
		status_task,    // Pointer to the task function
		"STATUS_TASK",    // Debug name string (Max 16 chars)
		4096,              // Stack size in BYTES (Note: Vanilla FreeRTOS uses words, ESP32 uses bytes)
		NULL,              // Pointer to pass parameters (NULL if none)
		1,                 // Task priority (Higher number = Higher priority)
		NULL              // Task handle pointer (NULL if not needed)
	);

	while(1)
	{
		
		vTaskDelay(pdMS_TO_TICKS(20));
	}

	// xTaskCreate(
	// 	gps_task,    // Pointer to the task function
	// 	"GPS_TASK",    // Debug name string (Max 16 chars)
	// 	4096,              // Stack size in BYTES (Note: Vanilla FreeRTOS uses words, ESP32 uses bytes)
	// 	NULL,              // Pointer to pass parameters (NULL if none)
	// 	1,                 // Task priority (Higher number = Higher priority)
	// 	NULL              // Task handle pointer (NULL if not needed)
	// );
	

		
	
	// lcd_init();
	

	int buttons[] = {BUT1_PIN,1,BUT2_PIN,1}; 

	for(int i=0; i < 4; i+=2)
	{
		gpio_reset_pin(buttons[i]);
		gpio_set_direction(buttons[i], GPIO_MODE_INPUT);
		gpio_set_pull_mode(buttons[i], GPIO_PULLUP_ONLY);
	}
	int butlvl;
	while(1)
	{
		for(int i=0; i < 4;i+=2)
		{
			butlvl = gpio_get_level(buttons[i]);
			if(!butlvl && buttons[i+1])
			{
				button_pressed(i >> 1);
			}

			buttons[i+1] = butlvl;
		}
		vTaskDelay(pdMS_TO_TICKS(20));
	}
	
}

