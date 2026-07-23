#pragma once
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#define LED_PIN_R	33
#define LED_PIN_G	32
#define BUTTON_0 15
#define BUTTON_1 16


typedef struct {
	uint8_t led;
} devstate_t;


extern devstate_t* DSTATE;