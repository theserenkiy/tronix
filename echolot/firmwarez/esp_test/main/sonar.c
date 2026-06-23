#include "sonar.h"
#include "chirp.h"
#include "wav.h"
#include <string.h>
#include "esp_log.h"
#include "esp_adc/adc_continuous.h"
#include "hal/adc_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static adc_continuous_handle_t adc_handle = NULL;

uint16_t *sonar_buffer;


void sonar_init(uint16_t *buf)
{
	sonar_buffer = buf;

	gpio_reset_pin(MOSDRV_ENA_PIN);
	gpio_set_direction(MOSDRV_ENA_PIN, GPIO_MODE_OUTPUT);
	gpio_set_level(MOSDRV_ENA_PIN, 0);

	gpio_reset_pin(DCDC_ENA_PIN);
	gpio_set_direction(DCDC_ENA_PIN, GPIO_MODE_OUTPUT);
	gpio_set_level(DCDC_ENA_PIN, 0);

	sonar_adc_init();
	chirp_init();
}

void sonar_precharge(int ms)
{
	sonar_charge(1);
	vTaskDelay(pdMS_TO_TICKS(ms));
}

void sonar_charge(int state)
{
	gpio_set_level(DCDC_ENA_PIN, state);
}

void sonar_tx_prepare()
{
	sonar_precharge(MT_PRECHARGE_DELAY_MS);
	gpio_set_level(MOSDRV_ENA_PIN, 1);
}

void sonar_tx_done()
{
	esp_rom_delay_us(IR_DSBL_DELAY_US);
	gpio_set_level(MOSDRV_ENA_PIN, 0); 
	sonar_charge(0);
}

void sonar_ping(uint16_t *buf, int ntimes)
{
	for(int i=0; i < ntimes; i++)
	{
		sonar_tx_prepare();
		chirp_fire();
		sonar_tx_done();

		sonar_adc_capture(buf, ADC_RECORD_SAMPLES);
		buf += ADC_RECORD_SAMPLES;
	}
}


esp_err_t sonar_adc_init(void)
{
	adc_continuous_handle_cfg_t adc_config = {
		.max_store_buf_size = 32768,
		.conv_frame_size = 1024,
	};

	ESP_ERROR_CHECK(
		adc_continuous_new_handle(&adc_config, &adc_handle)
	);

	adc_digi_pattern_config_t pattern = {
		.atten     = ADC_ATTEN_DB_0,
		.channel   = ADC_CHANNEL_USED,
		.unit      = ADC_UNIT_USED,
		.bit_width = ADC_BITWIDTH_12,
	};

	adc_continuous_config_t dig_cfg = {
		.sample_freq_hz = ADC_SAMPLE_FREQ_HZ,
		.conv_mode      = ADC_CONV_SINGLE_UNIT_1,
		.format         = ADC_DIGI_OUTPUT_FORMAT_TYPE1,
		.pattern_num    = 1,
		.adc_pattern    = &pattern,
	};

	ESP_ERROR_CHECK(
		adc_continuous_config(adc_handle, &dig_cfg)
	);

	return ESP_OK;
}


esp_err_t sonar_adc_capture(uint16_t *buffer, size_t samples)
{
	//printf("ADC record started...\n");

	uint8_t dma_buf[1024];

	size_t collected = 0;

	ESP_ERROR_CHECK(
		adc_continuous_start(adc_handle)
	);

	while (collected < samples)
	{
		uint32_t bytes_read = 0;

		esp_err_t ret =
			adc_continuous_read(
				adc_handle,
				dma_buf,
				sizeof(dma_buf),
				&bytes_read,
				1000);

		if (ret != ESP_OK)
			continue;

		uint32_t results =
			bytes_read / sizeof(adc_digi_output_data_t);

		adc_digi_output_data_t *p =
			(adc_digi_output_data_t *)dma_buf;

		for (uint32_t i = 0;
			 i < results && collected < samples;
			 i++)
		{
			buffer[collected++] = p[i].type1.data;
		}
	}

	ESP_ERROR_CHECK(
		adc_continuous_stop(adc_handle)
	);

	printf("ADC record done\n");

	return ESP_OK;
}




