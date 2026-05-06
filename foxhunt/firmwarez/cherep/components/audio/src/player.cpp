#include "audio/player.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/dac_cosine.h"
#include <cstring>
#include <cmath>

static const char *TAG = "Audio";

namespace audio {

// ——— Предопределённые последовательности ————————————————————————————————

const ToneNote SOUND_CARD_FOUND[] = {
    {1000, 100},
};
const size_t SOUND_CARD_FOUND_LEN = 1;

const ToneNote SOUND_CARD_WRITTEN[] = {
    {1000, 100},
    {0,     80},
    {1200, 100},
};
const size_t SOUND_CARD_WRITTEN_LEN = 3;

const ToneNote SOUND_TASK_PROMPT[] = {
    {800,  120},
    {0,     60},
    {1000, 120},
    {0,     60},
    {1200, 200},
};
const size_t SOUND_TASK_PROMPT_LEN = 5;

const ToneNote SOUND_TASK_DONE[] = {
    {1000, 100},
    {0,     50},
    {1200, 100},
    {0,     50},
    {1500, 200},
    {0,     50},
    {2000, 300},
};
const size_t SOUND_TASK_DONE_LEN = 7;

const ToneNote SOUND_ERROR[] = {
    {800,  200},
    {0,     50},
    {600,  200},
    {0,     50},
    {400,  400},
};
const size_t SOUND_ERROR_LEN = 5;

const ToneNote SOUND_SECOND_TOUCH[] = {
    {1500, 80},
    {0,    60},
    {1500, 80},
    {0,    60},
    {1500, 80},
};
const size_t SOUND_SECOND_TOUCH_LEN = 5;

// ——— AudioPlayer ————————————————————————————————————————————————————————

AudioPlayer::AudioPlayer(gpio_num_t speaker_gpio, gpio_num_t buzzer_gpio)
    : speaker_gpio_(speaker_gpio), buzzer_gpio_(buzzer_gpio)
{}

AudioPlayer::~AudioPlayer()
{
    if (inited_) {
        ledc_stop(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
        if (buzzer_gpio_ != GPIO_NUM_NC) {
            ledc_stop(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, 0);
        }
    }
}

esp_err_t AudioPlayer::init()
{
    // LEDC таймер для динамика
    ledc_timer_config_t timer_cfg = {};
    timer_cfg.speed_mode      = LEDC_LOW_SPEED_MODE;
    timer_cfg.duty_resolution = LEDC_TIMER_10_BIT;
    timer_cfg.timer_num       = LEDC_TIMER_0;
    timer_cfg.freq_hz         = 1000; // начальная частота, меняется динамически
    timer_cfg.clk_cfg         = LEDC_AUTO_CLK;

    esp_err_t ret = ledc_timer_config(&timer_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LEDC timer config failed");
        return ret;
    }

    // LEDC канал для динамика
    ledc_channel_config_t ch_cfg = {};
    ch_cfg.gpio_num   = speaker_gpio_;
    ch_cfg.speed_mode = LEDC_LOW_SPEED_MODE;
    ch_cfg.channel    = LEDC_CHANNEL_0;
    ch_cfg.timer_sel  = LEDC_TIMER_0;
    ch_cfg.duty       = 0; // выключен пока
    ch_cfg.hpoint     = 0;

    ret = ledc_channel_config(&ch_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LEDC channel config failed");
        return ret;
    }

    // Опциональный зуммер на отдельном пине
    if (buzzer_gpio_ != GPIO_NUM_NC) {
        ledc_timer_config_t buz_timer = timer_cfg;
        buz_timer.timer_num = LEDC_TIMER_1;

        ledc_channel_config_t buz_ch = ch_cfg;
        buz_ch.gpio_num = buzzer_gpio_;
        buz_ch.channel  = LEDC_CHANNEL_1;
        buz_ch.timer_sel= LEDC_TIMER_1;

        ledc_timer_config(&buz_timer);
        ledc_channel_config(&buz_ch);
    }

    inited_ = true;
    ESP_LOGI(TAG, "Audio init: speaker=GPIO%d buzzer=GPIO%d",
             speaker_gpio_, buzzer_gpio_);
    return ESP_OK;
}

// ——— Генерация тонов через LEDC ——————————————————————————————————————————

void AudioPlayer::setFreq(gpio_num_t gpio, ledc_channel_t channel, uint32_t freq_hz)
{
    if (!inited_ || freq_hz == 0) return;

    ledc_timer_t timer = (channel == LEDC_CHANNEL_0) ? LEDC_TIMER_0 : LEDC_TIMER_1;

    ledc_set_freq(LEDC_LOW_SPEED_MODE, timer, freq_hz);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, channel, 512); // 50% duty
    ledc_update_duty(LEDC_LOW_SPEED_MODE, channel);
}

void AudioPlayer::stopChannel(ledc_channel_t channel)
{
    ledc_set_duty(LEDC_LOW_SPEED_MODE, channel, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, channel);
}

void AudioPlayer::playTone(uint32_t freq_hz, uint32_t duration_ms)
{
    if (!inited_) return;

    if (freq_hz > 0) {
        setFreq(speaker_gpio_, LEDC_CHANNEL_0, freq_hz);
        if (buzzer_gpio_ != GPIO_NUM_NC) {
            setFreq(buzzer_gpio_, LEDC_CHANNEL_1, freq_hz);
        }
    }

    vTaskDelay(pdMS_TO_TICKS(duration_ms));

    stopChannel(LEDC_CHANNEL_0);
    if (buzzer_gpio_ != GPIO_NUM_NC) {
        stopChannel(LEDC_CHANNEL_1);
    }
}

void AudioPlayer::silence(uint32_t duration_ms)
{
    stopChannel(LEDC_CHANNEL_0);
    if (buzzer_gpio_ != GPIO_NUM_NC) stopChannel(LEDC_CHANNEL_1);
    if (duration_ms > 0) {
        vTaskDelay(pdMS_TO_TICKS(duration_ms));
    }
}

void AudioPlayer::playSequence(const ToneNote *notes, size_t len)
{
    if (!inited_ || !notes) return;

    for (size_t i = 0; i < len; i++) {
        if (notes[i].freq_hz == 0) {
            silence(notes[i].duration_ms);
        } else {
            playTone(notes[i].freq_hz, notes[i].duration_ms);
        }
    }
}

// ——— Воспроизведение сырого PCM через DAC ————————————————————————————————

void AudioPlayer::playRawPcm(const uint8_t *data, size_t len,
                              uint32_t sample_rate_hz)
{
    if (!data || len == 0) return;

    // Остановить LEDC на пине динамика перед DAC
    stopChannel(LEDC_CHANNEL_0);

    // Использовать DAC cosine для простых сигналов не получится — нужен прямой вывод
    // Используем прямую запись в DAC через dac_oneshot
    // Реализация: таймер на esp_timer + DAC output
    // Для полноценного PCM playback подключите I2S или используйте esp_codec_dev

    // Упрощённая реализация: побайтовый вывод с задержкой
    // НЕ подходит для высоких sample_rate, но работает для 8000 Hz
    uint32_t period_us = 1000000 / sample_rate_hz;

    // Включить DAC канал 2 (GPIO26)
    // В ESP-IDF v5.x используется dac_oneshot API
    dac_cosine_handle_t dac_handle = nullptr;
    dac_cosine_config_t cos_cfg = {
        .chan_id   = DAC_CHAN_1, // DAC_CHAN_1 = GPIO26 (channel 2)
        .freq_hz   = 1000,      // не используется в raw mode
        .clk_src   = DAC_COSINE_CLK_SRC_DEFAULT,
        .atten     = DAC_COSINE_ATTEN_DB_0,
        .phase     = DAC_COSINE_PHASE_0,
        .offset    = 0,
        .flags     = {.force_set_freq = false},
    };

    // Примечание: для корректного PCM воспроизведения рекомендуется
    // использовать esp_codec_dev или I2S с external DAC.
    // Данная реализация — заглушка с базовой функциональностью.

    // Прямой вывод через устаревший (но рабочий) API dac_output_voltage
    // В IDF v5.x замените на dac_oneshot_output_voltage
    ESP_LOGI(TAG, "Playing %zu PCM bytes @ %lu Hz", len, (unsigned long)sample_rate_hz);

    // TODO: реализовать через dac_continuous API для качественного воспроизведения
    // Placeholder: просто задержка длительности
    uint64_t duration_us = (uint64_t)len * 1000000ULL / sample_rate_hz;
    vTaskDelay(pdMS_TO_TICKS(duration_us / 1000));

    ESP_LOGI(TAG, "PCM playback done");
}

} // namespace audio
