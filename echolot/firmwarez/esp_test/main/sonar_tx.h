#pragma once
#include "common.h"

void sonar_tx_init(void);

void sonar_precharge(int ms);

void sonar_charge(int state);

void sonar_tx_burst(uint32_t cycles, int need_osc_suppression);
