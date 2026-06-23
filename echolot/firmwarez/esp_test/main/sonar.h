#pragma once

// #include <stdint.h>
#include "common.h"
#include <stddef.h>
#include "esp_err.h"

void sonar_init();

void sonar_precharge(int ms);

void sonar_charge(int state);

void sonar_tx_prepare();

void sonar_tx_done();

void sonar_ping(int freq_start, int freq_end, int duration_us, int ntimes);

esp_err_t sonar_adc_init(void);
esp_err_t sonar_adc_capture(uint16_t *buffer, size_t samples);
