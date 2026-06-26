#include "common.h"
#include "ui.h"
#include "views.h"
#include "encoder.h"

int buttons[] = {BUT0_PIN,1,BUT1_PIN,1}; 
int cur_view_idx = 0;
view_t *cur_view = NULL;


view_t views[] = {
	{
		.title = "MAIN",
		.update = view_main_update,
		.on_event = view_main_on_event,
		.init = NULL,
		.deinit = NULL
	},
	{
		.title = "OSC",
		.update = view_osc_update,
		.on_event = view_osc_on_event,
		.init = view_osc_init,
		.deinit = NULL
	}
};

int view_count = 2;



QueueHandle_t event_queue;



void ui_init()
{
	lcd_init();
	encoder_init();

	for(int i=0; i < 4; i+=2)
	{
		gpio_reset_pin(buttons[i]);
		gpio_set_direction(buttons[i], GPIO_MODE_INPUT);
		gpio_set_pull_mode(buttons[i], GPIO_PULLUP_ONLY);
	}

	ui_switch_view(0);

	event_queue = xQueueCreate(10, sizeof(event_t));

	xTaskCreate(
		ui_button_task,    // Pointer to the task function
		"BUTTON_TASK",    // Debug name string (Max 16 chars)
		4096,              // Stack size in BYTES (Note: Vanilla FreeRTOS uses words, ESP32 uses bytes)
		NULL,              // Pointer to pass parameters (NULL if none)
		1,                 // Task priority (Higher number = Higher priority)
		NULL              // Task handle pointer (NULL if not needed)
	);

	xTaskCreate(
		encoder_task,    // Pointer to the task function
		"ENCODER_TASK",    // Debug name string (Max 16 chars)
		4096,              // Stack size in BYTES (Note: Vanilla FreeRTOS uses words, ESP32 uses bytes)
		NULL,              // Pointer to pass parameters (NULL if none)
		1,                 // Task priority (Higher number = Higher priority)
		NULL              // Task handle pointer (NULL if not needed)
	);

	xTaskCreate(
		ui_task,    // Pointer to the task function
		"UI_TASK",    // Debug name string (Max 16 chars)
		16384,              // Stack size in BYTES (Note: Vanilla FreeRTOS uses words, ESP32 uses bytes)
		NULL,              // Pointer to pass parameters (NULL if none)
		1,                 // Task priority (Higher number = Higher priority)
		NULL              // Task handle pointer (NULL if not needed)
	);

	if(!DSTAT->sd_ok)
	{
		DSTAT->ui_blocked = 1;
		lcd_fill_screen(0);
		lcd_text_format(4,COLOR_WHITE,COLOR_RED,10);
		lcd_origin(10,10);
		lcd_draw_string("No SD");
		lcd_redraw();
	}
}

void ui_task(void *prm)
{
	event_t ev;
	int ret, skip;
	while(1)
	{
		skip = 0;
		while(xQueueReceive(event_queue, &ev, 20 / portTICK_PERIOD_MS) == pdPASS) {
            // ui_print_event("Event received",&ev);
			if(skip)
				continue;
			skip = 1;
			ret = cur_view->on_event(&ev);
			if(ev.type=='B' && ev.value==1 && !ret)
			{
				ui_next_view();
			}
        }

		vTaskDelay(pdMS_TO_TICKS(20));
	}
	
}

void ui_button_task(void *prm)
{
	int butlvl;
	while(1)
	{
		vTaskDelay(pdMS_TO_TICKS(50));
		if(DSTAT->ui_blocked)
			continue;
		for(int i=0; i < 4;i+=2)
		{
			butlvl = gpio_get_level(buttons[i]);
			if(!butlvl && buttons[i+1])
				ui_send_event('B',i >> 1);

			buttons[i+1] = butlvl;
		}
		
	}
}

void ui_send_event(char type, int value)
{
	event_t ev;
	ev.type = type;
	ev.value = value;
	// ui_print_event("Send event",&ev);
	xQueueSend(event_queue, (void *)&ev, 0);
}

void ui_print_event(const char *prefix, event_t *ev)
{
	printf("%s: type=%c; value=%d\n", prefix, ev->type, ev->value);
}

void ui_next_view()
{
	ui_switch_view((cur_view_idx+1)%view_count);
}

void ui_update_view()
{
	lcd_fill_screen(0);
	cur_view->update();
	lcd_redraw();
}

void ui_switch_view(int view_idx)
{
	if(cur_view && cur_view->deinit)
	{
		cur_view->deinit();
	}
	if(view_idx >= view_count)
	{
		printf("ERROR: view %d not exists!\n",view_idx);
		return;
	}
	cur_view_idx = view_idx;
	cur_view = &views[view_idx];
	if(cur_view->init)
		cur_view->init();
		
	ui_update_view();
}

void ui_sleep(int state)
{
	DSTAT->ui_blocked = state;
	lcd_sleep(state);
	if(!state)
		ui_update_view();
}

void ui_show_error(char *s)
{
	lcd_text_format(2,COLOR_WHITE,COLOR_DARKRED,3);
	lcd_origin(0,26);
	lcd_wl(s);
}

