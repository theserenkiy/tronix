#pragma once

typedef struct {
	char type;
	int value;
} event_t;

typedef struct {
	char title[16];
	void (*update)() ;
	int (*on_event)(event_t*);
} view_t;


void ui_init();

void ui_task(void *prm);

void ui_button_task(void *prm);

void ui_send_event(char type, int value);

void ui_print_event(const char *prefix, event_t *ev);

void ui_next_view();

void ui_update_view();

void ui_switch_view(int view);

void ui_sleep(int state);
