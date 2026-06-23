#pragma once
#include <stdio.h>
#include <inttypes.h>
#include <string.h>



FILE *wav_open(char *fname, int nsamp, uint32_t fs, char *info);
