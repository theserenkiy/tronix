#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s_std.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include <math.h>
#include "esp_task_wdt.h"

static const char *TAG = "AUDIO_REC_PLAY";

// Пины для MAX98357 (воспроизведение)
#define I2S_BCK_PIN         26  // BCLK
#define I2S_WS_PIN          27  // LRCK
#define I2S_DOUT_PIN        25  // DIN

// Пин для аналогового микрофона

// Пин для аналогового микрофона - ОБЯЗАТЕЛЬНО указываем канал ADC
#define MIC_ADC_CHANNEL     ADC_CHANNEL_6   // GPIO34 соответствует ADC1_CHANNEL_6
#define MIC_ADC_PIN         GPIO_NUM_34       // Для информации

// Параметры звука
#define SAMPLE_RATE         16000
#define RECORD_SECONDS      5
#define BUF_COUNT           4
#define BUF_LEN             1024

// Расчет размера буфера для записи
#define TOTAL_SAMPLES       (SAMPLE_RATE * RECORD_SECONDS)
#define AUDIO_BUFFER_SIZE   (TOTAL_SAMPLES * sizeof(int16_t))

static int16_t *audio_recording = NULL;
static adc_oneshot_unit_handle_t adc_handle = NULL;
static i2s_chan_handle_t tx_handle = NULL;  // Только для воспроизведения

// ========== ИНИЦИАЛИЗАЦИЯ АЦП ДЛЯ МИКРОФОНА ==========
// ========== ИНИЦИАЛИЗАЦИЯ АЦП ДЛЯ МИКРОФОНА ==========
void init_adc_mic(void) {
    ESP_LOGI(TAG, "Инициализация АЦП для микрофона (GPIO %d, канал %d)", MIC_ADC_PIN, MIC_ADC_CHANNEL);
    
    // Настройка АЦП
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));
    
    // Настройка канала АЦП - используем константу канала, а не GPIO!
    adc_oneshot_chan_cfg_t channel_config = {
        .atten = ADC_ATTEN_DB_12,   // Диапазон 0-3.6V (для микрофона)
        .bitwidth = ADC_BITWIDTH_12, // 12 бит (0-4095)
    };
    // ВАЖНО: передаем ADC1_CHANNEL_6, а не GPIO_NUM_34
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, MIC_ADC_CHANNEL, &channel_config));
    
    ESP_LOGI(TAG, "АЦП инициализирован (12 бит, атенюация 11dB)");
}

// ========== ЗАПИСЬ С АЦП (без DMA, но достаточно для 16 кГц) ==========
void record_audio(void) {
    ESP_LOGI(TAG, "Начинаем запись на %d секунд...", RECORD_SECONDS);
    ESP_LOGI(TAG, "Требуется памяти: %d байт (%.1f КБ)", AUDIO_BUFFER_SIZE, AUDIO_BUFFER_SIZE / 1024.0);
    
    // Выделяем память под запись
    audio_recording = (int16_t*)malloc(AUDIO_BUFFER_SIZE);
    if (audio_recording == NULL) {
        ESP_LOGE(TAG, "Ошибка выделения памяти!");
        return;
    }
    
    // Расчет интервала семплирования (16 кГц -> 62.5 мкс)
    const uint32_t sample_interval_us = 1000000 / SAMPLE_RATE;  // 62.5 мкс
    
    int adc_raw = 0;
    int prev_progress = -1;
    
    for (int i = 0; i < TOTAL_SAMPLES; i++) {
        // Читаем АЦП (блокирующий вызов - ~30 мкс)
        esp_err_t ret = adc_oneshot_read(adc_handle, 6, &adc_raw);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Ошибка чтения АЦП на семпле %d", i);
            break;
        }
        
        // Преобразование 12-bit (0-4095) в 16-bit знаковый (-32768..32767)
        // Смещаем так, чтобы тишина (2050) давала ~0
        audio_recording[i] = (int16_t)((adc_raw - 2048) * 16);
        
        // Точная задержка для поддержания частоты дискретизации
        if (i < TOTAL_SAMPLES - 1) {
            esp_rom_delay_us(sample_interval_us);
        }
        
        // Прогресс
        int progress = (i * 100) / TOTAL_SAMPLES;
        if (progress != prev_progress && (progress % 10 == 0 || progress == 1)) {
            ESP_LOGI(TAG, "Запись: %d%% (%d/%d семплов)", progress, i, TOTAL_SAMPLES);
            prev_progress = progress;
        }

		if (i % 1000 == 0) {
            esp_task_reset_wdt();
        }
    }
    
    ESP_LOGI(TAG, "Запись завершена. Записано %d семплов", TOTAL_SAMPLES);
}

// ========== ИНИЦИАЛИЗАЦИЯ I2S ДЛЯ MAX98357 (МОНО-РЕЖИМ) ==========
void init_i2s_speaker(void) {
    ESP_LOGI(TAG, "Инициализация I2S для MAX98357 (моно-режим)");
    
    // Настройка канала (Master, Tx)
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    chan_cfg.dma_desc_num = BUF_COUNT;
    chan_cfg.dma_frame_num = BUF_LEN;
    chan_cfg.auto_clear = true;  // Убираем шумы при переполнении буфера
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &tx_handle, NULL));
    
    // Настройка стандартного режима I2S (Philips) для MAX98357
    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
        // КЛЮЧЕВОЙ МОМЕНТ: I2S_SLOT_MODE_MONO - автоматически дублирует моно-данные на оба канала
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = I2S_BCK_PIN,
            .ws = I2S_WS_PIN,
            .dout = I2S_DOUT_PIN,
            .din = I2S_GPIO_UNUSED,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };
    
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_handle, &std_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(tx_handle));
    
    ESP_LOGI(TAG, "MAX98357 инициализирован в МОНО-режиме (автодублирование каналов)");
}

// ========== ВОСПРОИЗВЕДЕНИЕ (БЕЗ ДОПОЛНИТЕЛЬНОГО БУФЕРА) ==========
void play_audio(void) {
    if (audio_recording == NULL) {
        ESP_LOGE(TAG, "Нет аудиоданных для воспроизведения!");
        return;
    }
    
    ESP_LOGI(TAG, "Начинаем воспроизведение...");
    ESP_LOGI(TAG, "ВНИМАНИЕ: I2S в МОНО-режиме автоматически дублирует звук на оба канала");
    
    size_t bytes_written = 0;
    size_t offset = 0;
    int prev_progress = -1;
    
    // Пишем моно-данные напрямую - драйвер сам продублирует их на левый и правый каналы! [citation:1][citation:5]
    while (offset < AUDIO_BUFFER_SIZE) {
        size_t bytes_to_write = BUF_LEN * sizeof(int16_t);  // Пишем пачками
        if (bytes_to_write > (AUDIO_BUFFER_SIZE - offset)) {
            bytes_to_write = AUDIO_BUFFER_SIZE - offset;
        }
        
        esp_err_t ret = i2s_channel_write(tx_handle, 
                                         (uint8_t*)audio_recording + offset, 
                                         bytes_to_write, 
                                         &bytes_written, 
                                         portMAX_DELAY);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Ошибка записи: %s", esp_err_to_name(ret));
            break;
        }
        
        offset += bytes_written;
        
        // Прогресс
        int progress = (offset * 100) / AUDIO_BUFFER_SIZE;
        if (progress != prev_progress && (progress % 10 == 0 || progress == 1)) {
            ESP_LOGI(TAG, "Воспроизведение: %d%%", progress);
            prev_progress = progress;
        }
    }
    
    ESP_LOGI(TAG, "Воспроизведение завершено (%d байт)", offset);
}

// ========== MAIN ==========
void app_main(void) {
    ESP_LOGI(TAG, "=== СИСТЕМА ЗАПИСИ И ВОСПРОИЗВЕДЕНИЯ (АЦП + MAX98357) ===");
    ESP_LOGI(TAG, "Параметры: %d Гц, 16 бит, %d секунд", SAMPLE_RATE, RECORD_SECONDS);
    
    // 1. Инициализация АЦП для микрофона
    init_adc_mic();
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // 2. Запись 5 секунд
    record_audio();
    
    // 3. Деинициализация АЦП (освобождаем ресурсы)
    adc_oneshot_del_unit(adc_handle);
    adc_handle = NULL;
    
    // 4. Инициализация I2S для MAX98357
    init_i2s_speaker();
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // 5. Воспроизведение (без выделения дополнительной памяти!)
    play_audio();
    
    // 6. Очистка
    if (audio_recording != NULL) {
        free(audio_recording);
        audio_recording = NULL;
    }
    
    i2s_channel_disable(tx_handle);
    i2s_del_channel(tx_handle);
    
    ESP_LOGI(TAG, "=== РАБОТА ЗАВЕРШЕНА ===");
}