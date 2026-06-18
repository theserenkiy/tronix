#include "common.h"
#include "lcd2.h"



void lcd2_init(void)
{

	gpio_reset_pin(LCD_LED_PIN);
	gpio_set_direction(LCD_LED_PIN, GPIO_MODE_OUTPUT);
	gpio_set_drive_capability(LCD_LED_PIN, GPIO_DRIVE_CAP_2);
	gpio_set_level(LCD_LED_PIN, 1);

	// Конфигурация дисплея
	st7735_config_t cfg = {
		.cs_io_num   = LCD_CS_PIN,
		.dc_io_num   = LCD_DC_PIN,
		.rst_io_num  = LCD_RST_PIN,
		.bl_io_num   = -1,  // -1 = не используется
		.host_id     = SPI2_HOST,     // используем SPI2
	};

	
	ESP_LOGI("ST7735", "Initializing display...");
	esp_err_t ret = st7735_init(&cfg);
	
	if (ret != ESP_OK) {
		ESP_LOGE("ST7735", "Display initialization failed!");
		return;
	}
	
	ESP_LOGI("ST7735", "Display initialized successfully");
	
	// Очистка экрана черным цветом
	st7735_fill_screen(ST7735_BLACK);
	
	
	xTaskCreate(
		lcd2_task,    // Pointer to the task function
		"LCD_TASK",    // Debug name string (Max 16 chars)
		4096,              // Stack size in BYTES (Note: Vanilla FreeRTOS uses words, ESP32 uses bytes)
		NULL,              // Pointer to pass parameters (NULL if none)
		1,                 // Task priority (Higher number = Higher priority)
		NULL              // Task handle pointer (NULL if not needed)
	);
}

void lcd2_update()
{
	{
		uint16_t color = DEVSTATUS->sd_ok ? ST7735_DARKGREEN : ST7735_RED;
		st7735_fill_rect(0,0,16,11,color);
		st7735_draw_string(2, 2, "SD", ST7735_WHITE, color, 1);
	}

	// {
	// 	uint16_t color = DEVSTATUS->gps_ok ? ST7735_DARKBLUE : ST7735_RED;
	// 	st7735_fill_rect(18,0,20,11,color);
	// 	st7735_draw_string(2, 22, "GPS", ST7735_WHITE, color, 1);
	// }

	// st7735_draw_string(40, 2, DEVSTATUS->time, ST7735_WHITE, 0, 1);
}

void lcd2_task(void *prm)
{
	

	while (1) {

		


		vTaskDelay(pdMS_TO_TICKS(1000));
	}

	vTaskDelete(NULL); 
}