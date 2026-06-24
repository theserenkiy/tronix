#pragma once

void sd_init();

void sd_speed_test(uint16_t *buf);

int sd_check();

void get_info(char *str);

FILE *sd_open_wav(int len);

// void sd_open_wav(FILE *fp, int len);

int sd_save_ping(uint16_t *buf, size_t len);

int get_max_file_number(void);
