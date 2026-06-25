#pragma once

void recorder_init();

void recorder_test();

void recorder_make_record();

typedef struct {
	char type;	// L - LFM, P - PSK
	uint16_t fstart_khz;
	uint16_t fstop_khz;
	uint16_t dur_us;
	//full sym length, including tho half-transitions on edges.
	//dur_us = psk_sym_dur*psk_sym_count
	uint8_t psk_sym_dur_us;
	//sym-to-sym transition 
	uint8_t psk_trans_us;
	//PSK data, 1bit=1symbol, MSB-first
	uint16_t psk_pattern;
	uint8_t psk_sym_count;
	uint8_t nbufs;
} record_cfg_t;
