#pragma once

#define TESTCFG_MAX_ITEMS	64

typedef struct {
	char type;	// L - LFM, P - PSK, F - one freq
	uint16_t fstart_khz;
	uint16_t fstop_khz;
	uint16_t dur_us;
	//full sym length, including tho half-transitions on edges.
	//dur_us = psk_sym_dur*psk_sym_count
	uint16_t psk_sym_dur_us;
	//sym-to-sym transition 
	uint16_t psk_trans_us;
	//PSK data, 1bit=1symbol, MSB-first
	uint16_t psk_pattern;
	uint8_t psk_sym_count;
	uint8_t npings;
} record_cfg_t;

extern record_cfg_t testcfg_records[TESTCFG_MAX_ITEMS];
extern uint8_t testcfg_records_len;

void recorder_init();

void recorder_test();

void recorder_record_by_config();

void recorder_make_record();
