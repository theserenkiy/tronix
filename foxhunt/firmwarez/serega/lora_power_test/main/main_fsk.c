//https://share.google/aimode/jEdlCsumRkKSgXMi4

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "driver/ledc.h"

// Пины подключения
#define PIN_MOSI    2
#define PIN_MISO    4
#define PIN_SCLK    15
#define PIN_CS      13
#define PIN_RST     14
#define DIO2_PIN		16


#define LEDC_TIMER            LEDC_TIMER_0
#define LEDC_MODE             LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL          LEDC_CHANNEL_0
#define LEDC_DUTY_RES         LEDC_TIMER_10_BIT // 10-bit resolution (0-1023)
#define LEDC_FREQUENCY        1000 // 1 kHz

// Частота работы
#define RF_FREQUENCY_HZ    433920000LLU

// Регистры SX1278
#define REG_FIFO                0x00
#define REG_OP_MODE             0x01
#define REG_BITRATE_MSB         0x02
#define REG_BITRATE_LSB         0x03
#define REG_FDEV_MSB            0x04
#define REG_FDEV_LSB            0x05
#define REG_FRF_MSB             0x06
#define REG_FRF_MID             0x07
#define REG_FRF_LSB             0x08
#define REG_PA_CONFIG           0x09
#define REG_PA_DAC				0x4D
#define REG_OCP                 0x0B
#define REG_LNA                 0x0C
#define REG_RX_CONFIG           0x0D
#define REG_FIFO_TX_BASE_ADDR   0x0E
#define REG_FIFO_RX_BASE_ADDR   0x0F
#define REG_FIFO_ADDR_PTR       0x0D
#define REG_PACKET_CONFIG_1     0x30
#define REG_PACKET_CONFIG_2     0x31
#define REG_PAYLOAD_LENGTH      0x22
#define REG_DIO_MAPPING_1       0x40
#define REG_DIO_MAPPING_2       0x41
#define REG_VERSION             0x42
#define REG_RSSI_VALUE          0x48
#define REG_IMAGE_CAL           0x5B
#define REG_TEMP                0x5C

// Режимы работы
#define MODE_FSK                0x00
#define MODE_TX                 0x03
#define MODE_STDBY              0x01
#define MODE_SLEEP              0x00

uint8_t powsteps[] = {2,6,10,14,17,20};

static const char *TAG = "SX1278_MIN";
static spi_device_handle_t spi;

// Запись регистра
void lora_write(uint8_t reg, uint8_t value) {
	uint8_t tx[2] = {reg | 0x80, value};
	spi_transaction_t t = {
		.length = 16,
		.tx_buffer = tx,
	};
	spi_device_transmit(spi, &t);
}

// Чтение регистра
uint8_t lora_read(uint8_t reg) {
	uint8_t tx[2] = {reg & 0x7F, 0};
	uint8_t rx[2] = {0};
	spi_transaction_t t = {
		.length = 16,
		.tx_buffer = tx,
		.rx_buffer = rx,
	};
	spi_device_transmit(spi, &t);
	return rx[1];
}

void lora_set_20dbm()
{
	lora_write(REG_PA_CONFIG, 0xFF); 
	lora_write(REG_PA_DAC, 0x87); 
	lora_write(REG_OCP, 0x2B);
}

void lora_set_power(int dbm)
{
	if(dbm >= 20)
		return lora_set_20dbm();
	
	lora_write(REG_PA_CONFIG, 0x80 + dbm-2); 
	lora_write(REG_PA_DAC, 0x84); 
	lora_write(REG_OCP, 0x2B);
}

void morse_init()
{
	 // 1. Настройка таймера LEDC
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .timer_num        = LEDC_TIMER,
        .duty_resolution  = LEDC_DUTY_RES,
        .freq_hz          = LEDC_FREQUENCY,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);

    // 2. Настройка канала LEDC
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL,
        .timer_sel      = LEDC_TIMER,
        .gpio_num       = DIO2_PIN,
        .duty           = 0, // Изначально выключен
        .hpoint         = 0
    };
    ledc_channel_config(&ledc_channel);

    // 3. Установка 50% скважности (меандр)
    // 10-bit res = 1024 шага. 50% = 1024 / 2 = 512
    
}

void morse_beep(int freq, int len, int pause)
{
	ledc_set_freq(LEDC_MODE, LEDC_TIMER, freq);
	ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 512);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);

	vTaskDelay(pdMS_TO_TICKS(len));

	ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 0);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);

	vTaskDelay(pdMS_TO_TICKS(pause));
}


void morse_on_power_step(int step)
{
	int dbm = powsteps[step];
	// lora_set_power(dbm);
	ESP_LOGI(TAG, "SET POWER: %d", dbm);

	for(int i=0; i < 3; i++)
	{
		morse_beep(800,500,100);
	}
	
	for(int i=0; i <= step; i++)
	{
		morse_beep(800,150,150);
	}
}

void app_main(void) {
	// 1. Инициализация GPIO для RST
	gpio_config_t rst_conf = {
		.pin_bit_mask = (1ULL << PIN_RST),
		.mode = GPIO_MODE_OUTPUT,
		.pull_up_en = GPIO_PULLUP_ENABLE,
	};
	gpio_config(&rst_conf);
	
	// Сброс SX1278
	gpio_set_level(PIN_RST, 0);
	vTaskDelay(pdMS_TO_TICKS(10));
	gpio_set_level(PIN_RST, 1);
	vTaskDelay(pdMS_TO_TICKS(50));
	
	// 2. Инициализация SPI
	spi_bus_config_t bus_cfg = {
		.mosi_io_num = PIN_MOSI,
		.miso_io_num = PIN_MISO,
		.sclk_io_num = PIN_SCLK,
		.quadwp_io_num = -1,
		.quadhd_io_num = -1,
	};
	
	spi_device_interface_config_t dev_cfg = {
		.clock_speed_hz = 10000,  // 10kHz
		.mode = 0,
		.spics_io_num = PIN_CS,
		.queue_size = 1,
	};
	
	ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_DISABLED));
	ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &dev_cfg, &spi));
	
	ESP_LOGI(TAG, "SPI initialized");
	

	morse_init();


	// 3. Проверка версии
	uint8_t version = lora_read(REG_VERSION);
	ESP_LOGI(TAG, "REG_VERSION = 0x%02X", version);
	
	if (version != 0x12) {
		ESP_LOGE(TAG, "SX1278 not detected! Expected 0x12, got 0x%02X", version);
		while (1) {
			vTaskDelay(pdMS_TO_TICKS(1000));
		}
	}
	
	ESP_LOGI(TAG, "SX1278 detected!");
	
	// 4. Настройка FSK режима
	lora_write(REG_OP_MODE, MODE_FSK);	//fsk
	// lora_write(REG_OP_MODE, 0x0b);		//ook
	vTaskDelay(pdMS_TO_TICKS(10));

	//мапаем DIO2
	lora_write(REG_DIO_MAPPING_1, 0b11 << 2);

	//continious mode
	lora_write(REG_PACKET_CONFIG_1, 0);
	lora_write(REG_PACKET_CONFIG_2, 0);

  
	// 5. Настройка частоты 433 MHz
	uint64_t frf = ((uint64_t)RF_FREQUENCY_HZ << 19) / 32000000;
	lora_write(REG_FRF_MSB, (frf >> 16) & 0xFF);
	lora_write(REG_FRF_MID, (frf >> 8) & 0xFF);
	lora_write(REG_FRF_LSB, frf & 0xFF);

	
	// 6. Настройка девиации частоты (20 kHz для более широкого сигнала)
	lora_write(REG_FDEV_MSB, 0x00);
	lora_write(REG_FDEV_LSB, 0x39); // 20 kHz девиации
	
	// 5. Настройка битрейта (2.4 kbps для медленного сигнала)
	// lora_write(REG_BITRATE_MSB, 0x68); // 1200 bps (minimum)
	// lora_write(REG_BITRATE_LSB, 0x2B);

	lora_write(REG_BITRATE_MSB, 0x00); // 1200 bps (minimum)
	lora_write(REG_BITRATE_LSB, 0x01);

	
	// 6. Настройка мощности 20 dBm
	lora_set_power(2);

	// sx1278_write_reg(REG_OCP, 0x1A);        // 50 mA
	
	// 7. Включение передатчика
	lora_write(REG_OP_MODE, MODE_TX);
	
	ESP_LOGI(TAG, "Transmitter ON at %.3f MHz, power 10 dBm", (double)RF_FREQUENCY_HZ / 1e6);
	ESP_LOGI(TAG, "SDR should see CW carrier");

	

	// 8. Бесконечный цикл
	while (1) {
		for(uint8_t step=0; step < 6; step++)
		{
			morse_on_power_step(step);
			vTaskDelay(pdMS_TO_TICKS(1000));
		}
		// Периодически читаем статус
		// uint8_t mode = lora_read(REG_OP_MODE);
		// ESP_LOGI(TAG, "Mode register: 0x%02X", mode);
	}
}