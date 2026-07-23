#pragma once
#include "common.h"
#include "ble_net.h"


extern ble_command_t ble_commands[];

int blec_get_state(void *buf, int len);

int blec_set_time(void *buf, int len);

int blec_led_toggle(void *buf, int len);


