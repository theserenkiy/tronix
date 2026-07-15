#include "ble_commands.h"
#include "led.h"

ble_command_t ble_commands[] = {
	{
		.code = 1,
		.name = "SET_TIME",
		.cb = blec_set_time
	},
	{
		.code = 2,
		.name = "LED_TOGGLE",
		.cb = blec_led_toggle
	},
	{0}
};


int blec_set_time(void* buf, int len)
{
	int* ibuf = (int*)buf;
	printf("SET TIME: %d\n",*ibuf);
	return 0;
}

int blec_led_toggle(void* buf, int len)
{
	uint8_t* cbuf = (uint8_t*)buf;
	printf("LED_TOGGLE: %d\n",*cbuf);
	led_set_levels(*cbuf);

	return 0;
}

