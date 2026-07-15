#include "ble_commands.h"
#include "led.h"

ble_command_t ble_commands[] = {
	{
		.code = 1,
		.name = "GET_STATE",
		.cb = blec_get_state
	},
	{
		.code = 2,
		.name = "SET_TIME",
		.cb = blec_set_time
	},
	{
		.code = 3,
		.name = "LED_TOGGLE",
		.cb = blec_led_toggle
	},
	{0}
};

#pragma pack(push, 1)

typedef struct {
	uint8_t led;
} resp_state_t;

#pragma pack(pop)

resp_state_t resp_state = {}

int blec_get_state(void* buf, int len)
{
	resp_state_t* rst = &resp_state;

	rst->led = DSTAT.led;
	ble_send()
	return 0;
}


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

