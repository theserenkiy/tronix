#pragma once
#include "common.h"

typedef struct {
	uint8_t code;
	char name[16];
	int (*cb)(void*, int);
} ble_command_t;


#include "ble_commands.h"

void blen_on_receive(void *buf, int len);
