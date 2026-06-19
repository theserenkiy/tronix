#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lib.h"
#include "common.h"

void delay_ms(int ms)
{
	vTaskDelay(pdMS_TO_TICKS(ms));
}
