#pragma once
#include <stdio.h>
#include <inttypes.h>
#include <string.h>

void save_wav(uint16_t *buf, int nsamp, char *fname, uint32_t fs, uint8_t bitshift);

FILE *open_wav(int nsamp, char *fname, uint32_t fs);
