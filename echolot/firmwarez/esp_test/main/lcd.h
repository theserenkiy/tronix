#pragma once

void lcd_init(void);

void lcd_hard_reset();

void lcd_init_registers();

void lcd_fill_screen(uint8_t R, uint8_t G, uint8_t B);
