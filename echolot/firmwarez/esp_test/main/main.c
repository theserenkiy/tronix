#include <stdio.h>
#include "common.h"

#include "sonar_adc.h"
#include "sonar_tx.h"

#define NO_TX	0

uint16_t buffer[ADC_RECORD_SAMPLES];

void app_main()
{
	if(!NO_TX)
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
	
	sonar_adc_init();


	vTaskDelay(pdMS_TO_TICKS(1000));

	int is_first = 1;
	int n = 0;
	while(1)
	{
		if(!NO_TX)
			sonar_tx_burst(32, 1);
		
		
		if(is_first)
		{
			sonar_adc_capture(buffer, ADC_RECORD_SAMPLES);
			sonar_uart_send_buffer(buffer, ADC_RECORD_SAMPLES);
			// break;
		}
		is_first = 0;
		n++;

		vTaskDelay(pdMS_TO_TICKS(BURST_TO_BURST_DELAY_MS));
	}
	
}

