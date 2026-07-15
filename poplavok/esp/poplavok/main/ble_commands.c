#include <stdio.h>
#include "ble_commands.h"


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


void blec_set_time(void* buf, int len)
{
	int* ibuf = (int*)buf;
	printf("SET TIME: %d\n",*ibuf);
}

void blec_led_toggle(void* buf, int len)
{
	char* cbuf = (char*)buf;
	printf("LED_TOGGLE: %s\n",cbuf);
}