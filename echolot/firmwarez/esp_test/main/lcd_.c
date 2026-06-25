#include <stdio.h>
#include "esp_log.h"
#include "driver/spi_master.h"
#include "lcd_.h"
#include "common.h"

#define LCD_PIXELS	LCD_ROWS * LCD_COLS

static spi_device_handle_t spi_handle = NULL;


static uint8_t framebuf[LCD_PIXELS*2];

void lcd_init(void)
{
	printf("Initializing LCD...\n");

	gpio_reset_pin(LCD_LED_PIN);
	gpio_set_direction(LCD_LED_PIN, GPIO_MODE_OUTPUT);
	gpio_set_drive_capability(LCD_LED_PIN, GPIO_DRIVE_CAP_1);
	gpio_set_level(LCD_LED_PIN, 1);

	gpio_reset_pin(LCD_RST_PIN);
	gpio_set_direction(LCD_RST_PIN, GPIO_MODE_OUTPUT);
	gpio_set_level(LCD_RST_PIN, 0);

	// gpio_reset_pin(LCD_CS_PIN);
	// gpio_set_direction(LCD_CS_PIN, GPIO_MODE_OUTPUT);
	// gpio_set_level(LCD_CS_PIN, 1);

	gpio_reset_pin(LCD_DC_PIN);
	gpio_set_direction(LCD_DC_PIN, GPIO_MODE_OUTPUT);

	

	spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = LCD_SPI_SPEED,
        .mode = 0,                          // SPI mode 0 (CPOL=0, CPHA=0)
        .spics_io_num = LCD_CS_PIN,         // Номер пина CS (или -1 если управляем вручную)
        .cs_ena_pretrans = 0,               // Задержка перед активацией CS
        .cs_ena_posttrans = 0,              // Задержка после деактивации CS
        .queue_size = 7,                    // Размер очереди транзакций
        .flags = SPI_DEVICE_NO_DUMMY,                         // SPI_DEVICE_HALFDUPLEX и др.
    };
    
    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &dev_cfg, &spi_handle));
    printf("SPI device added successfully\n");


	// gpio_set_level(LCD_CS_PIN, 0);

	lcd_hard_reset();

	lcd_init_registers();
	printf("Display initialization completed\n");

	lcd_fill_screen(255,0,255);

	// gpio_set_level(LCD_CS_PIN, 1);

}

// Утилиты для отправки через SPI
static void spi_write_cmd(uint8_t cmd) {
	gpio_set_level(LCD_DC_PIN, 0); // Режим команды
	
	spi_transaction_t trans = {
		.length = 8,                    // Отправляем 8 бит (1 байт)
		.tx_buffer = &cmd
	};
	
	spi_device_polling_transmit(spi_handle, &trans);      // Отправка cmd
}

static void spi_write_data(uint8_t *data, size_t len) {
	gpio_set_level(LCD_DC_PIN, 1); // Режим данных

	spi_transaction_t trans = {
		.length = 8 * len,
		.tx_buffer = data
	};

	spi_device_polling_transmit(spi_handle, &trans);      // Отправка data
}

static void spi_write_byte(uint8_t byte) {
	spi_write_data(&byte, 1);
}

static void spi_write_conf(uint8_t cmd, uint8_t* data, size_t len) {
	spi_write_cmd(cmd);
	spi_write_data(data, len);
}

// Простейшая последовательность инициализации
void lcd_hard_reset() {
	printf("HARD RESET\n");
	gpio_set_level(LCD_RST_PIN, 1);
	vTaskDelay(pdMS_TO_TICKS(200));
	gpio_set_level(LCD_RST_PIN, 0);
	vTaskDelay(pdMS_TO_TICKS(200));
	gpio_set_level(LCD_RST_PIN, 1);
	vTaskDelay(pdMS_TO_TICKS(200)); // Ожидание после выхода из сброса [citation:6]
}

void lcd_init_registers() {
	
	printf("SWRESET\n");
	spi_write_cmd(0x01);	// SWRESET 
	vTaskDelay(pdMS_TO_TICKS(200));

	printf("SLPOUT\n");
	spi_write_cmd(0x11); // SLPOUT - Выход из сна
	vTaskDelay(pdMS_TO_TICKS(500)); // Задержка после SLPOUT обязательна [citation:6]

	printf("CONFIG\n");
	// Frame rate
	spi_write_cmd(0xB1);
	uint8_t d0[] = {0x01, 0x2C, 0x2D};
	spi_write_data(d0,3);

	spi_write_cmd(0xB2);
	uint8_t d1[] = {0x01, 0x2C, 0x2D};
	spi_write_data(d1,3);

	spi_write_cmd(0xB3);
	uint8_t d2[] = {0x01, 0x2C, 0x2D, 0x01, 0x2C, 0x2D};
	spi_write_data(d2,6);

	// Inversion
	spi_write_cmd(0xB4);
	spi_write_byte(0x07);

	// Power
	uint8_t d3[] = {0xA2, 0x02, 0x84};
	spi_write_conf(0xC0, d3, 3);

	spi_write_cmd(0xC1);
	spi_write_byte(0xC5);

	uint8_t d4[] = {0x0A, 0x00};
	spi_write_conf(0xC2, d4, 2);

	uint8_t d5[] = {0x8A, 0x2A};
	spi_write_conf(0xC3, d5, 2);

	uint8_t d6[] = {0x8A, 0xEE};
	spi_write_conf(0xC4, d6, 2);

	spi_write_cmd(0xC5);
	spi_write_byte(0x0E);

	
	// Color inversion
	spi_write_cmd(0x21);


	spi_write_cmd(0x36); // MADCTL - Настройка осей и порядка цвета
	// !!!! or C8
	spi_write_byte(0xC0); // MY=1, MX=1, MV=0, BGR=1 (типичное значение)

	spi_write_cmd(0x3A); // COLMOD - Установка цветового режима
	spi_write_byte(0x05); // 16 бит на пиксель (RGB565) [citation:6]

	{ 
		uint8_t d[] = {0x02,0x1C,0x07,0x12,0x37,0x32,0x29,0x2D,0x29,0x25,0x2B,0x39,0x00,0x01,0x03,0x10}; 
		spi_write_conf(0xE0,d,16); 
	}
	{ 
		uint8_t d[] = {0x03,0x1D,0x07,0x06,0x2E,0x2C,0x29,0x2D,0x2E,0x2E,0x37,0x3F,0x00,0x00,0x02,0x10}; 
		spi_write_conf(0xE1,d,16); 
	}

	printf("NORON\n");
	spi_write_cmd(0x13);	// NORON
	vTaskDelay(pdMS_TO_TICKS(100));

	printf("DISPON\n");
	spi_write_cmd(0x29); // DISPON - Включение дисплея
	vTaskDelay(pdMS_TO_TICKS(100));
}

void lcd_set_window(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
    // Добавляем хардварное смещение дисплея 80х160
    x1 += 26; x2 += 26;
    y1 += 1;  y2 += 1;


    // Столбцы (Column Address Set)
    spi_write_cmd(0x2A);
    uint8_t data_x[] = {0, x1, 0, x2};
    spi_write_data(data_x, 4);

    // Строки (Row Address Set)
    spi_write_cmd(0x2B);
    uint8_t data_y[] = {0, y1, 0, y2};
    spi_write_data(data_y, 4);

	spi_write_cmd(0x2C); // RAMWR - команда начала записи в RAM
}

inline void packColor(uint8_t R, uint8_t G, uint8_t B, uint8_t* color)
{
	color[0] = (R & 0b11111000) | (G >> 5);
	color[1] = ((G & 0b11111100) << 3) | (B >> 3);
}

inline void putPix(uint8_t* color, uint8_t* buf)
{
	*buf = color[0];
	*(buf+1) = color[1];
}

// Заполнение экрана цветом
void lcd_fill_screen(uint8_t R, uint8_t G, uint8_t B) {
	
	printf("Filling the screen...\n");

	// gpio_set_level(LCD_CS_PIN, 0);
	// RGB565
	// uint8_t color_bytes_0[] = {255,255};
	// uint8_t color_bytes_1[] = {0,0};
	// { 
	// 	(R & 0b11111000) | (G >> 5), 
	// 	((G & 0b11111100) << 3) | (B >> 3)
	// };

	uint8_t col1[2], col2[2];
	packColor(255,0,0,col1);
	packColor(0,255,0,col2);

	for (uint32_t i = 0; i < LCD_PIXELS; i++) {
		framebuf[i] = (i & 8) ? 0xFFFF : 0x0000;
	}

	lcd_set_window(0,0,79,159);
	
	spi_write_data((uint8_t*)framebuf, 2*LCD_PIXELS);
	printf("Screen fill completed!\n");

	// gpio_set_level(LCD_CS_PIN, 1);
}