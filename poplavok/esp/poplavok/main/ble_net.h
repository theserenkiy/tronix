#pragma once
#include "common.h"
#include "ble.h"

typedef struct {
	uint8_t code;
	char name[16];
	int (*cb)(void*, int);
} ble_command_t;


#include "ble_commands.h"

void blen_on_receive(void *buf, int len);

void blen_send(void *buf, int len);
