#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s_std.h"
#include "esp_log.h"
#include <math.h>

static const char *TAG = "MAX98357";

// Пины подключения MAX98357
#define I2S_BCK_PIN     26
#define I2S_WS_PIN      27
#define I2S_DIN_PIN     25

// Параметры звука
#define SAMPLE_RATE     16000  // Частота дискретизации 16 кГц
#define SINE_FREQ       1000   // Частота синуса 1 кГц
#define AMPLITUDE       0.5    // Амплитуда (0.0 - 1.0)
#define DURATION_SEC    10     // Длительность воспроизведения (сек)

// Размер буфера для DMA
#define DMA_BUF_COUNT   4
#define DMA_BUF_LEN     1024

// Генерация синуса
void generate_sine_wave(int16_t *buffer, size_t samples, float frequency, float amplitude, float sample_rate) {
    float angular_freq = 2.0f * M_PI * frequency / sample_rate;
    
    for (size_t i = 0; i < samples; i++) {
        // Генерируем синус
        float sample_float = amplitude * sinf(angular_freq * i);
        
        // Преобразуем в 16-bit целое (диапазон -32768..32767)
        int16_t sample_int = (int16_t)(sample_float * 32767.0f);
        
        // Для MAX98357 нужны два одинаковых канала (левый и правый)
        buffer[i * 2] = sample_int;     // Левый канал
        buffer[i * 2 + 1] = sample_int; // Правый канал
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "Инициализация I2S для MAX98357");
    
    // Настройка I2S канала
    i2s_chan_handle_t tx_handle = NULL;
    
    // Базовые настройки I2S
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    chan_cfg.dma_desc_num = DMA_BUF_COUNT;
    chan_cfg.dma_frame_num = DMA_BUF_LEN / 4;  // 4 байта на стерео сэмпл (2 канала * 2 байта)
    
    esp_err_t ret = i2s_new_channel(&chan_cfg, &tx_handle, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Ошибка создания I2S канала: %s", esp_err_to_name(ret));
        return;
    }
    
    // Настройка стандартного режима I2S (Philips)
    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = I2S_BCK_PIN,
            .ws = I2S_WS_PIN,
            .dout = I2S_DIN_PIN,
            .din = I2S_GPIO_UNUSED,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };
    
    ret = i2s_channel_init_std_mode(tx_handle, &std_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Ошибка инициализации I2S стандартным режимом: %s", esp_err_to_name(ret));
        i2s_del_channel(tx_handle);
        return;
    }
    
    // Включение I2S
    ret = i2s_channel_enable(tx_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Ошибка включения I2S: %s", esp_err_to_name(ret));
        i2s_del_channel(tx_handle);
        return;
    }
    
    ESP_LOGI(TAG, "I2S инициализирован, частота дискретизации: %d Гц", SAMPLE_RATE);
    ESP_LOGI(TAG, "Генерация синуса %d Гц в течение %d секунд...", SINE_FREQ, DURATION_SEC);
    
    // Расчет размера буфера
    size_t samples_per_buffer = DMA_BUF_LEN / 4;  // 4 байта на стерео сэмпл
    int16_t *audio_buffer = malloc(DMA_BUF_LEN);
    if (audio_buffer == NULL) {
        ESP_LOGE(TAG, "Ошибка выделения памяти");
        i2s_channel_disable(tx_handle);
        i2s_del_channel(tx_handle);
        return;
    }
    
    // Генерация и отправка данных
    int total_buffers = (SAMPLE_RATE * DURATION_SEC) / samples_per_buffer;
    size_t bytes_written;
    
    for (int buf_count = 0; buf_count < total_buffers; buf_count++) {
        // Генерируем блок синуса
        generate_sine_wave(audio_buffer, samples_per_buffer, SINE_FREQ, AMPLITUDE, SAMPLE_RATE);
        
        // Отправляем в I2S
        ret = i2s_channel_write(tx_handle, audio_buffer, DMA_BUF_LEN, &bytes_written, portMAX_DELAY);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Ошибка записи в I2S: %s", esp_err_to_name(ret));
            break;
        }
        
        // Прогресс (каждую секунду)
        if (buf_count % (SAMPLE_RATE / samples_per_buffer) == 0) {
            ESP_LOGI(TAG, "Воспроизведение: %d сек / %d сек", 
                     buf_count * samples_per_buffer / SAMPLE_RATE, DURATION_SEC);
        }
    }
    
    ESP_LOGI(TAG, "Воспроизведение завершено");
    
    // Очистка
    free(audio_buffer);
    i2s_channel_disable(tx_handle);
    i2s_del_channel(tx_handle);
    
    ESP_LOGI(TAG, "Работа завершена");
}