#pragma once
#include "st7735.h"

void lcd2_init(void);

void lcd2_update();

void lcd2_sleep(int state);

void lcd2_waveform(uint16_t *buf, int samples, int toggle);

