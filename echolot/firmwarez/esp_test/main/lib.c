#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lib.h"
#include "common.h"

inline void delay_ms(int ms)
{
	vTaskDelay(pdMS_TO_TICKS(ms));
}


inline void delay_us(int us)
{
	esp_rom_delay_us(us);
}