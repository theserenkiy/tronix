#include <stdio.h>
#include "common.h"

#include "sonar_adc.h"
#include "sonar_tx.h"


uint16_t buffer[ADC_RECORD_SAMPLES];

void app_main()
{
	sonar_tx_init();



	while(1)
	{
		// sonar_tx_burst(32);
		
		sonar_adc_init();
		sonar_adc_capture(buffer, ADC_RECORD_SAMPLES);

		vTaskDelay(pdMS_TO_TICKS(BURST_TO_BURST_DELAY_MS));
	}
	
}