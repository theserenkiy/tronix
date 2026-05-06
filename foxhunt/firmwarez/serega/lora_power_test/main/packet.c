#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_common.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_check.h"
    
// Пины подключения
#define PIN_MOSI    2
#define PIN_MISO    4
#define PIN_SCLK    15
#define PIN_CS      13
#define PIN_RST     14

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
#define REG_OCP                 0x0B
#define REG_LNA                 0x0C
#define REG_RX_CONFIG           0x0D
#define REG_FIFO_TX_BASE_ADDR   0x0E
#define REG_FIFO_RX_BASE_ADDR   0x0F
#define REG_FIFO_ADDR_PTR       0x0D
#define REG_PACKET_CONFIG_1     0x1D
#define REG_PAYLOAD_LENGTH      0x22
#define REG_DIO_MAPPING_1       0x40
#define REG_VERSION             0x42
#define REG_RSSI_VALUE          0x48

// Режимы работы
#define MODE_FSK                0x00
#define MODE_TX                 0x03
#define MODE_STDBY              0x01

// Log тег
static const char *TAG = "SX1278_FSK";

static spi_device_handle_t spi_handle;

// Функция записи в регистр
void lora_write(uint8_t reg, uint8_t value) {
    uint8_t tx_buffer[2] = {reg | 0x80, value};
    spi_transaction_t trans = {
        .length = 8 * 2,
        .tx_buffer = tx_buffer,
        .rx_buffer = NULL,
        .flags = SPI_TRANS_USE_TXDATA  // Используем TXDATA для маленьких транзакций
    };
    spi_device_transmit(spi_handle, &trans);
}

// Функция чтения регистра
uint8_t lora_read(uint8_t reg) {
    uint8_t tx_buffer[2] = {reg & 0x7F, 0x00};
    uint8_t rx_buffer[2] = {0, 0};
    spi_transaction_t trans = {
        .length = 8 * 2,
        .tx_buffer = tx_buffer,
        .rx_buffer = rx_buffer,
        .flags = SPI_TRANS_USE_RXDATA
    };
    spi_device_transmit(spi_handle, &trans);
    return rx_buffer[1];
}

// Функция установки режима
void sx1278_set_mode(uint8_t mode) {
    uint8_t op_mode = lora_read(REG_OP_MODE);
    op_mode = (op_mode & 0xF8) | mode;
    lora_write(REG_OP_MODE, op_mode);
    vTaskDelay(pdMS_TO_TICKS(10));
}

// Функция установки мощности (0-20 dBm, FSK режим)
void sx1278_set_power(int8_t dbm) {
    uint8_t pa_config = 0;
    
    if (dbm <= 13) {
        // Используем PA0 (до +13 dBm)
        pa_config = (dbm + 5) & 0x1F;
        pa_config |= 0x00;
    } else {
        // Используем PA1 и PA-BOOST (до +20 dBm)
        pa_config = 0x80; // Включить PA-BOOST
        int output_power;
        
        if (dbm >= 20) {
            output_power = 0x0F;
        } else if (dbm >= 17) {
            output_power = 0x0C;
        } else if (dbm >= 15) {
            output_power = 0x0B;
        } else {
            output_power = 0x0A;
        }
        pa_config |= output_power & 0x0F;
        pa_config |= 0x40; // PA1 включен
    }
    
    lora_write(REG_PA_CONFIG, pa_config);
    ESP_LOGI(TAG, "Power set to %d dBm (PA_CONFIG: 0x%02X)", dbm, pa_config);
    
    // Настройка защиты от перегрузки для высоких мощностей
    if (dbm > 17) {
        lora_write(REG_OCP, 0x3B); // 150mA
    } else if (dbm > 13) {
        lora_write(REG_OCP, 0x2C); // 100mA
    } else {
        lora_write(REG_OCP, 0x1A); // 50mA
    }
}

// Инициализация SX1278 в режиме FSK
void sx1278_init_fsk(void) {
    ESP_LOGI(TAG, "Initializing SX1278 in FSK mode...");
    
    // Сброс
    gpio_set_level(PIN_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(PIN_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(50));
    
    // Проверка версии
    uint8_t version = lora_read(REG_VERSION);
    ESP_LOGI(TAG, "SX1278 Version: 0x%02X (expected 0x12)", version);
    
    if (version != 0x12) {
        ESP_LOGW(TAG, "Unexpected version!");
    }
    
    // Установка режима FSK
    lora_write(REG_OP_MODE, MODE_FSK);
    vTaskDelay(pdMS_TO_TICKS(10));
    
    // Настройка битрейта (4.8 kbps)
    lora_write(REG_BITRATE_MSB, 0x1A);
    lora_write(REG_BITRATE_LSB, 0x6B);
    
    // Настройка частотного девиатора (5 kHz)
    lora_write(REG_FDEV_MSB, 0x00);
    lora_write(REG_FDEV_LSB, 0x28);
    
    // Настройка радиочастоты (433 MHz)
    uint32_t frf = 433000000LLU;
    uint64_t frf_calc = ((uint64_t)frf << 19) / 32000000;
    lora_write(REG_FRF_MSB, (frf_calc >> 16) & 0xFF);
    lora_write(REG_FRF_MID, (frf_calc >> 8) & 0xFF);
    lora_write(REG_FRF_LSB, frf_calc & 0xFF);
    
    // Настройка пакетного режима
    lora_write(REG_PACKET_CONFIG_1, 0x40);
    
    // База FIFO
    lora_write(REG_FIFO_TX_BASE_ADDR, 0x00);
    lora_write(REG_FIFO_RX_BASE_ADDR, 0x00);
    
    // Настройка LNA
    lora_write(REG_LNA, 0x23);
    
    // Переключение в режим ожидания
    sx1278_set_mode(MODE_STDBY);
    
    ESP_LOGI(TAG, "FSK initialization complete");
}

// Передача данных
void sx1278_send_data(uint8_t *data, uint8_t len) {
    if (len > 64) len = 64;
    
    // Очистка FIFO
    lora_write(REG_FIFO_ADDR_PTR, 0x00);
    
    // Запись данных в FIFO
    for (int i = 0; i < len; i++) {
        lora_write(REG_FIFO, data[i]);
    }
    
    // Установка длины пакета
    lora_write(REG_PAYLOAD_LENGTH, len);
    
    // Переключение в режим передачи
    sx1278_set_mode(MODE_TX);
    
    ESP_LOGI(TAG, "Transmitting %d bytes: %.*s", len, len, data);
    
    // Ожидание окончания передачи (увеличено для надежности)
    vTaskDelay(pdMS_TO_TICKS(200));
    
    // Возврат в режим ожидания
    sx1278_set_mode(MODE_STDBY);
}

// Задача переключения мощности
void tx_power_cycling_task(void *pvParameters) {
    int8_t power_levels[] = {2, 5, 10, 15, 20};
    const char *messages[] = {
        "FSK 433MHz - 2dBm",
        "FSK 433MHz - 5dBm",
        "FSK 433MHz - 10dBm",
        "FSK 433MHz - 15dBm",
        "FSK 433MHz - 20dBm"
    };
    
    int index = 0;
    
    ESP_LOGI(TAG, "Starting FSK transmission with power cycling");
    
    while (1) {
        // Установка мощности
        sx1278_set_power(power_levels[index]);
        
        // Передача сообщения каждые 500 мс в течение 2 секунд
        for (int i = 0; i < 4; i++) {
            sx1278_send_data((uint8_t*)messages[index], strlen(messages[index]));
            vTaskDelay(pdMS_TO_TICKS(500));
        }
        
        ESP_LOGI(TAG, "=== Power level %d dBm finished, switching to next ===", 
                 power_levels[index]);
        
        index = (index + 1) % 5;
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// Инициализация SPI
void spi_init(void) {
    // Конфигурация SPI шины без DMA
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_MOSI,
        .miso_io_num = PIN_MISO,
        .sclk_io_num = PIN_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 64,
        .flags = SPICOMMON_BUSFLAG_MASTER  // Флаг мастер-режима
    };
    
    // Конфигурация SPI устройства для полнодуплексного режима
    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = 2 * 1000 * 1000,  // Уменьшено до 2 MHz для стабильности
        .mode = 0,                          // SPI mode 0 (CPOL=0, CPHA=0)
        .spics_io_num = PIN_CS,
        .queue_size = 5,
        .flags = 0,                         // Убираем флаг HALFDUPLEX
        .pre_cb = NULL,
        .post_cb = NULL
    };
    
    // Инициализация SPI шины с DMA = 0 (отключено)
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_DISABLED));
    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &dev_cfg, &spi_handle));
    
    ESP_LOGI(TAG, "SPI initialized successfully");
}

// Инициализация GPIO
void gpio_init(void) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << PIN_RST),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    
    // Настройка CS пина как выход с высоким уровнем по умолчанию
    gpio_config_t cs_conf = {
        .pin_bit_mask = (1ULL << PIN_CS),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&cs_conf);
    gpio_set_level(PIN_CS, 1);
    
    gpio_set_level(PIN_RST, 1);
}

void app_main(void) {
    ESP_LOGI(TAG, "Starting SX1278 FSK transmitter");
    
    gpio_init();
    spi_init();
    sx1278_init_fsk();
    
    // Запуск задачи переключения мощности
    xTaskCreate(tx_power_cycling_task, "tx_power_task", 4096, NULL, 5, NULL);
}