#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_common.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"

// Пины подключения
#define PIN_MOSI    2
#define PIN_MISO    4
#define PIN_SCLK    15
#define PIN_CS      13
#define PIN_RST     14
#define PIN_DIO2    16  // DIO2 для выхода модулированного сигнала

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
#define REG_PACKET_CONFIG_2     0x1E
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

// Log тег
static const char *TAG = "SX1278_CW";

static spi_device_handle_t spi_handle;

// Функция записи в регистр
void lora_write(uint8_t reg, uint8_t value) {
    uint8_t tx_buffer[2] = {reg | 0x80, value};
    spi_transaction_t trans = {
        .length = 8 * 2,
        .tx_buffer = tx_buffer,
        .rx_buffer = NULL,
        .flags = SPI_TRANS_USE_TXDATA
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

// Функция установки мощности
void sx1278_set_power(int8_t dbm) {
    uint8_t pa_config = 0;
    
    if (dbm <= 13) {
        pa_config = (dbm + 5) & 0x1F;
    } else {
        pa_config = 0x80; // PA-BOOST включен
        int output_power;
        if (dbm >= 20) output_power = 0x0F;
        else if (dbm >= 17) output_power = 0x0C;
        else if (dbm >= 15) output_power = 0x0B;
        else output_power = 0x0A;
        pa_config |= output_power & 0x0F;
        pa_config |= 0x40; // PA1 включен
    }
    
    lora_write(REG_PA_CONFIG, pa_config);
    ESP_LOGI(TAG, "Power set to %d dBm (PA_CONFIG: 0x%02X)", dbm, pa_config);
    
    // Настройка OCP
    if (dbm > 17) lora_write(REG_OCP, 0x3B);
    else if (dbm > 13) lora_write(REG_OCP, 0x2C);
    else lora_write(REG_OCP, 0x1A);
}

// Настройка CW режима с выводом на DIO2
void sx1278_enable_cw_mode(void) {
    ESP_LOGI(TAG, "Configuring Continuous Wave mode with DIO2 output...");
    
    // 1. Переход в режим сна для переконфигурации
    sx1278_set_mode(MODE_SLEEP);
    vTaskDelay(pdMS_TO_TICKS(10));
    
    // 2. Настройка DIO2 как выход модулированного сигнала (FSK)
    // DIO2 может выводить: 0x00 - ClkOut, 0x01 - TxData (модулированный сигнал)
    lora_write(REG_DIO_MAPPING_2, 0x01); // DIO2 = TxData (FSK)
    
    // 3. Настройка FSK режима без пакетной обработки
    lora_write(REG_OP_MODE, MODE_FSK);
    vTaskDelay(pdMS_TO_TICKS(10));
    
    // 4. Отключаем пакетный режим, переходим в непрерывный режим передачи
    lora_write(REG_PACKET_CONFIG_1, 0x00); // Continuous mode без CRC
    lora_write(REG_PACKET_CONFIG_2, 0x00); // Без синхронизации слов
    
    // 5. Настройка битрейта (2.4 kbps для медленного сигнала)
    lora_write(REG_BITRATE_MSB, 0x34); // 32000 / 2400 = 13.33 -> 0x34
    lora_write(REG_BITRATE_LSB, 0x30);
    
    // 6. Настройка девиации частоты (20 kHz для более широкого сигнала)
    lora_write(REG_FDEV_MSB, 0x00);
    lora_write(REG_FDEV_LSB, 0xA0); // 20 kHz девиации
    
    // 7. Настройка частоты 433 МГц
    uint32_t frf = 433000000LLU;
    uint64_t frf_calc = ((uint64_t)frf << 19) / 32000000;
    lora_write(REG_FRF_MSB, (frf_calc >> 16) & 0xFF);
    lora_write(REG_FRF_MID, (frf_calc >> 8) & 0xFF);
    lora_write(REG_FRF_LSB, frf_calc & 0xFF);
    
    // 8. Настройка LNA для стабильности
    lora_write(REG_LNA, 0x23);
    
    // 9. Включение непрерывной передачи (Tx) без данных из FIFO
    // Для CW режима нужно, чтобы передатчик был активен даже без данных
    sx1278_set_mode(MODE_TX);
    
    ESP_LOGI(TAG, "CW mode enabled - carrier is ON continuously");
    ESP_LOGI(TAG, "DIO2 will output modulated data");
}

// Более простой метод: просто включаем несущую (без модуляции)
void sx1278_enable_carrier_only(void) {
    ESP_LOGI(TAG, "Enabling pure carrier (CW) - no modulation...");
    
    // Переход в сон
    sx1278_set_mode(MODE_SLEEP);
    vTaskDelay(pdMS_TO_TICKS(10));
    
    // Настройка FSK режима
    lora_write(REG_OP_MODE, MODE_FSK);
    vTaskDelay(pdMS_TO_TICKS(10));
    
    // Минимальный битрейт для псевдо-CW
    lora_write(REG_BITRATE_MSB, 0xFF);
    lora_write(REG_BITRATE_LSB, 0xFF);
    
    // Минимальная девиация
    lora_write(REG_FDEV_MSB, 0x00);
    lora_write(REG_FDEV_LSB, 0x01);
    
    // Частота 433 МГц
    uint32_t frf = 433000000LLU;
    uint64_t frf_calc = ((uint64_t)frf << 19) / 32000000;
    lora_write(REG_FRF_MSB, (frf_calc >> 16) & 0xFF);
    lora_write(REG_FRF_MID, (frf_calc >> 8) & 0xFF);
    lora_write(REG_FRF_LSB, frf_calc & 0xFF);
    
    // Отключаем пакетный режим
    lora_write(REG_PACKET_CONFIG_1, 0x00);
    
    // Настройка DIO2 на вывод тактового сигнала
    lora_write(REG_DIO_MAPPING_2, 0x03); // DIO2 = ClkOut
    
    // Включаем передатчик
    sx1278_set_mode(MODE_TX);
    
    ESP_LOGI(TAG, "Pure carrier ON");
}

// Генерация тестового сигнала на DIO2
void sx1278_send_raw_data_on_dio2(uint8_t *data, uint8_t len, bool repeat) {
    if (len > 64) len = 64;
    
    // Очистка FIFO
    lora_write(REG_FIFO_ADDR_PTR, 0x00);
    
    // Запись сырых данных в FIFO
    for (int i = 0; i < len; i++) {
        lora_write(REG_FIFO, data[i]);
    }
    
    lora_write(REG_PAYLOAD_LENGTH, len);
    
    if (repeat) {
        // Включаем режим повторения пакета
        lora_write(REG_PACKET_CONFIG_2, 0x08); // Restart RX on PacketCollision
    }
    
    // Переход в режим передачи (несущая уже включена)
    sx1278_set_mode(MODE_TX);
}

// Задача переключения мощности с непрерывной несущей
void cw_power_cycling_task(void *pvParameters) {
    int8_t power_levels[] = {2, 5, 10, 15, 20};
    int index = 0;
    
    ESP_LOGI(TAG, "Starting CW mode with power cycling every 2 seconds");
    
    // Включаем CW режим с несущей
    sx1278_enable_cw_mode();
    
    // Тестовые данные для отправки через DIO2
    uint8_t test_data[] = {0xAA, 0x55, 0xAA, 0x55, 0x00, 0xFF, 0x01, 0x02, 0x03, 0x04};
    
    while (1) {
        // Меняем мощность
        sx1278_set_power(power_levels[index]);
        
        ESP_LOGI(TAG, "=== Power: %d dBm for 2 seconds ===", power_levels[index]);
        
        // Отправляем сырые данные на DIO2 несколько раз за 2 секунды
        for (int i = 0; i < 4; i++) {
            sx1278_send_raw_data_on_dio2(test_data, sizeof(test_data), false);
            ESP_LOGI(TAG, "Sent raw data on DIO2");
            vTaskDelay(pdMS_TO_TICKS(500));
        }
        
        index = (index + 1) % 5;
        
        // Небольшая задержка между сменой мощности
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// Упрощенная задача - только несущая без данных
void simple_cw_power_cycling_task(void *pvParameters) {
    int8_t power_levels[] = {2, 5, 10, 15, 20};
    int index = 0;
    
    ESP_LOGI(TAG, "Starting pure carrier with power cycling");
    
    // Включаем чистую несущую
    sx1278_enable_carrier_only();
    
    while (1) {
        // Меняем мощность каждые 2 секунды
        sx1278_set_power(power_levels[index]);
        ESP_LOGI(TAG, "=== Carrier power: %d dBm ===", power_levels[index]);
        
        vTaskDelay(pdMS_TO_TICKS(2000));
        
        index = (index + 1) % 5;
    }
}

// Инициализация SPI
void spi_init(void) {
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_MOSI,
        .miso_io_num = PIN_MISO,
        .sclk_io_num = PIN_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 64,
        .flags = SPICOMMON_BUSFLAG_MASTER
    };
    
    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = 2 * 1000 * 1000,
        .mode = 0,
        .spics_io_num = PIN_CS,
        .queue_size = 5,
        .flags = 0
    };
    
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_DISABLED));
    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &dev_cfg, &spi_handle));
    
    ESP_LOGI(TAG, "SPI initialized");
}

// Инициализация GPIO
void gpio_init(void) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << PIN_RST) | (1ULL << PIN_CS),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    
    // Настройка DIO2 как входа (для вывода модулированного сигнала)
    gpio_config_t dio2_conf = {
        .pin_bit_mask = (1ULL << PIN_DIO2),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&dio2_conf);
    
    gpio_set_level(PIN_CS, 1);
    gpio_set_level(PIN_RST, 1);
    
    ESP_LOGI(TAG, "GPIO initialized, DIO2 configured as output for modulated signal");
}

void app_main(void) {
    ESP_LOGI(TAG, "Starting SX1278 Continuous Wave mode");
    
    gpio_init();
    spi_init();
    
    // Проверка связи с SX1278
    uint8_t version = lora_read(REG_VERSION);
    ESP_LOGI(TAG, "SX1278 Version: 0x%02X", version);
    
    if (version == 0x12) {
        ESP_LOGI(TAG, "SX1278 detected successfully");
        
        // Используйте один из двух режимов:
        
        // Вариант 1: CW с модулированными данными на DIO2
        xTaskCreate(cw_power_cycling_task, "cw_task", 4096, NULL, 5, NULL);
        
        // Вариант 2: Чистая несущая (без модуляции)
        // xTaskCreate(simple_cw_power_cycling_task, "simple_cw_task", 4096, NULL, 5, NULL);
    } else {
        ESP_LOGE(TAG, "SX1278 not detected! Check wiring.");
    }
}