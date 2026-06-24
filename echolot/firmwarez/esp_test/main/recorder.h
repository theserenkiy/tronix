#pragma once

void recorder_init();

void recorder_test();

void recorder_make_record();

typedef struct {
	uint16_t fstart_khz;
	uint16_t fstop_khz;
	uint16_t dur_us;
	uint8_t nbufs;
} record_info_t;
