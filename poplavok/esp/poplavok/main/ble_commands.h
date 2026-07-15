#pragma once
#include <inttypes.h>

void blec_set_time(void *buf, int len);

void blec_led_toggle(void *buf, int len);

typedef struct {
	uint8_t code;
	char name[16];
	void (*cb)(void*, int);
} ble_command_t;


extern ble_command_t ble_commands[];