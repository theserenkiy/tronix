#pragma once
#include "st7735.h"

#define COLOR(C)(uint16_t)(((C >> 16) & 0xF8)  | ((C << 3) & 0xE000) | ((C >> 13) & 0x7) | ((C << 5) & 0x1F00))

#define COLOR_RED			COLOR(0xFF0000)
#define COLOR_DARKRED		COLOR(0x770000)
#define COLOR_GREEN			COLOR(0x00FF00)
#define COLOR_DARKGREEN		COLOR(0x007700)
#define COLOR_BLUE			COLOR(0x0000FF)
#define COLOR_DARKBLUE		COLOR(0x000077)
#define COLOR_CYAN			COLOR(0x00FFFF)
#define COLOR_YELLOW		COLOR(0xFFFF00)
#define COLOR_DARKYELLOW	COLOR(0x777700)
#define COLOR_MAGENTA		COLOR(0xFF00FF)
#define COLOR_BLACK			0
#define COLOR_WHITE			COLOR(0xFFFFFF)


typedef struct {
	uint8_t x;
	uint8_t y;
	struct {
		uint8_t x; 
		uint8_t y; 
		uint8_t x1; 
		uint8_t y1;
		uint8_t w;
		uint8_t h;
	} last_block;
	uint8_t font_size;
	uint16_t font_color;
	uint16_t text_bg;
	uint8_t padding;
} lcd_conf_t;

extern lcd_conf_t *lcdc;

void lcd_init(void);

void lcd_update();

void lcd_sleep(int state);

void lcd_waveform(uint16_t *buf, int samples, int toggle);

void lcd_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);

void lcd_fill_screen(uint16_t color);

void lcd_text_format(uint8_t size, uint16_t color, uint16_t bg, uint8_t padding);

void lcd_redraw();

void lcd_origin(int x, int y);

void lcd_stack_right(uint8_t dx, uint8_t dy);

void lcd_stack_down(uint8_t dx, uint8_t dy);

uint16_t lcd_mk_color(int C);

uint16_t lcd_mk_gray(uint8_t C);

void lcd_color_test();

void lcd_draw_string(const char *str);

void lcd_wl(const char *str, ...);

void lcd_draw_osc(int len);
