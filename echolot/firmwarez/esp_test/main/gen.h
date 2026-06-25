#pragma once

void gen_init();

void gen_psk(int freq, float sym_dur, float trans_dur, uint16_t data_bits, int sym_len);

int gen_chirp_continue(int freq_start, int freq_end, float duration, double *phase, int *bit_num);

void gen_chirp(int freq_start, int freq_end, float duration);

void gen_fire();
