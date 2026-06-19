#pragma once

void sd_init();

int sd_check();

void get_info(char *str);

int sd_save_ping(uint16_t *buf, size_t len);

int get_max_file_number(void);
