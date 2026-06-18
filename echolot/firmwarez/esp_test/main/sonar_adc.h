#pragma once

// #include <stdint.h>
#include "common.h"
#include <stddef.h>
#include "esp_err.h"



esp_err_t sonar_adc_init(void);
esp_err_t sonar_adc_capture(uint16_t *buffer, size_t samples);
