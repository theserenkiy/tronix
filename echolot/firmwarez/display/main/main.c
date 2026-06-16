#include <stdio.h>
#include "driver/gpio.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <inttypes.h>
#include "st7735.h"

// Пины согласно вашему подключению
#define PIN_NUM_MOSI   18   // SDA
#define PIN_NUM_SCLK   19   // SCL
#define PIN_NUM_CS     23   // CS
#define PIN_NUM_DC     21   // D/C
#define PIN_NUM_RST    22   // RST
#define PIN_NUM_BCKL   -1   // -1 если подсветка не управляется программно

#define LCD_LED_PIN		5

void app_main(void)
{

	gpio_reset_pin(LCD_LED_PIN);
	gpio_set_direction(LCD_LED_PIN, GPIO_MODE_OUTPUT);
	gpio_set_drive_capability(LCD_LED_PIN, GPIO_DRIVE_CAP_0);
	gpio_set_level(LCD_LED_PIN, 1);

    // Конфигурация дисплея
    st7735_config_t cfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .sclk_io_num = PIN_NUM_SCLK,
        .cs_io_num   = PIN_NUM_CS,
        .dc_io_num   = PIN_NUM_DC,
        .rst_io_num  = PIN_NUM_RST,
        .bl_io_num   = PIN_NUM_BCKL,  // -1 = не используется
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
    
    // Вывод текста
    st7735_draw_string(10, 30, "Hello World!", ST7735_WHITE, ST7735_BLACK, 2);
    
    // Основной цикл (пример: смена цвета фона)
    uint16_t colors[] = {ST7735_RED, ST7735_GREEN, ST7735_BLUE};
    int color_index = 0;
    
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        st7735_fill_screen(colors[color_index]);
        color_index = (color_index + 1) % 3;
        // Заново выводим текст поверх нового цвета
        st7735_draw_string(10, 30, "Hello World!", ST7735_WHITE, colors[color_index], 2);
    }
}