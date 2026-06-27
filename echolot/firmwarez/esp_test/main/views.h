#pragma once
#include "view_osc.h"

void view_main_update();

int view_main_on_event(event_t *ev);



void view_tconf_init();

void view_tconf_deinit();

void view_tconf_update();

int view_tconf_on_event(event_t *ev);

void view_freq_init();

void view_freq_deinit();

void view_freq_update();

int view_freq_on_event(event_t *ev);
