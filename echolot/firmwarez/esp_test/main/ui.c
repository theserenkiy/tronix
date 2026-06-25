#include "common.h"
#include "ui.h"
#include "encoder.h"

int buttons[] = {BUT1_PIN,1,BUT2_PIN,1}; 
int cur_view = 0;

typedef struct {
	char type;
	int value;
} queue_item_t;

QueueHandle_t xQueue;

void ui_init()
{
	encoder_init();

	for(int i=0; i < 4; i+=2)
	{
		gpio_reset_pin(buttons[i]);
		gpio_set_direction(buttons[i], GPIO_MODE_INPUT);
		gpio_set_pull_mode(buttons[i], GPIO_PULLUP_ONLY);
	}

	xQueue = xQueueCreate(10, sizeof(queue_item_t));

	xTaskCreate(
		ui_task,    // Pointer to the task function
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
		4096,              // Stack size in BYTES (Note: Vanilla FreeRTOS uses words, ESP32 uses bytes)
		NULL,              // Pointer to pass parameters (NULL if none)
		1,                 // Task priority (Higher number = Higher priority)
		NULL              // Task handle pointer (NULL if not needed)
	);
}

void ui_task(void *prm)
{
	while(1)
	{
		vTaskDelay(pdMS_TO_TICKS(20));
	}
	
}

void ui_button_task(void *prm)
{
	int butlvl;
	while(1)
	{
		for(int i=0; i < 4;i+=2)
		{
			butlvl = gpio_get_level(buttons[i]);
			if(!butlvl && buttons[i+1])
			{
				ui_on_button(i >> 1);
			}

			buttons[i+1] = butlvl;
		}
		vTaskDelay(pdMS_TO_TICKS(20));
	}
}

void ui_on_button(int num)
{
	printf("Button pressed: %d\n",num);
}

void ui_on_encoder(int direction)
{
	printf("Encoder changed: %d\n",direction);
}