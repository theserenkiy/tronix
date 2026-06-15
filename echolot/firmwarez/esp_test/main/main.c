#include <stdio.h>
#include "common.h"

#include "sonar_adc.h"
#include "sonar_tx.h"


uint16_t buffer[ADC_RECORD_SAMPLES];

void app_main()
{
	sonar_tx_init();
	sonar_adc_init();

	vTaskDelay(pdMS_TO_TICKS(1000));

	int is_first = 1;
	int n = 0;
	while(1)
	{
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

