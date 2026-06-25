#pragma once
#include "st7735.h"

#define COLOR(C)(((C >> 16) & 0xF8)  | ((C << 3) & 0xE000) | ((C >> 13) & 0x7) | ((C << 5) & 0x1F00))

void lcd_init(void);

void lcd_update();

void lcd_sleep(int state);

void lcd_waveform(uint16_t *buf, int samples, int toggle);

void lcd_redraw();

uint16_t lcd_mk_color(int C);

uint16_t lcd_mk_gray(uint8_t C);

void lcd_gray_test();

void lcd_draw_osc(int len);
