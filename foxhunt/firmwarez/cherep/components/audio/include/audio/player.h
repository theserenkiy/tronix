#pragma once

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include <cstdint>
#include <cstddef>

namespace audio {

// ——— Нота (для программной генерации тонов) ——————————————————————————————
struct ToneNote {
    uint32_t freq_hz;    // 0 = пауза
    uint32_t duration_ms;
};

// ——— Предопределённые последовательности звуков ——————————————————————————
// Реализованы как массивы ToneNote + длина

/** 1 короткий биип: карта найдена */
extern const ToneNote SOUND_CARD_FOUND[];
extern const size_t   SOUND_CARD_FOUND_LEN;

/** 2 биипа: запись выполнена (простой КП) */
extern const ToneNote SOUND_CARD_WRITTEN[];
extern const size_t   SOUND_CARD_WRITTEN_LEN;

/** 3 восходящих тона: призыв пройти испытание */
extern const ToneNote SOUND_TASK_PROMPT[];
extern const size_t   SOUND_TASK_PROMPT_LEN;

/** Мелодия завершения: испытание пройдено, прислони карту */
extern const ToneNote SOUND_TASK_DONE[];
extern const size_t   SOUND_TASK_DONE_LEN;

/** Нисходящий тон: ошибка */
extern const ToneNote SOUND_ERROR[];
extern const size_t   SOUND_ERROR_LEN;

/** 1 биип: ожидание второго касания */
extern const ToneNote SOUND_SECOND_TOUCH[];
extern const size_t   SOUND_SECOND_TOUCH_LEN;

/**
 * Аудиоплеер.
 *
 * Два режима работы:
 * 1. Тон через LEDC PWM (любой GPIO) — для зуммера/пищалки
 * 2. DAC Cosine (GPIO26) — для динамика с синусоидой
 *
 * Для простоты и совместимости с DAC выход на GPIO26
 * используется LEDC на том же пине через аппаратный канал.
 * Это позволяет генерировать чистые тоны без CPU-нагрузки.
 *
 * Заглушка buzzer_gpio позволяет опционально управлять
 * пассивным зуммером на отдельном пине (по умолчанию -1 = нет).
 */
class AudioPlayer {
public:
    /**
     * @param speaker_gpio  GPIO динамика (G26, DAC_CHANNEL_2)
     * @param buzzer_gpio   GPIO зуммера (-1 = нет зуммера)
     */
    explicit AudioPlayer(gpio_num_t speaker_gpio,
                         gpio_num_t buzzer_gpio = GPIO_NUM_NC);
    ~AudioPlayer();

    /** Инициализировать LEDC каналы. */
    esp_err_t init();

    /** Сыграть один тон (блокирующий вызов). */
    void playTone(uint32_t freq_hz, uint32_t duration_ms);

    /** Сыграть паузу (тишина). */
    void silence(uint32_t duration_ms);

    /** Сыграть последовательность нот. */
    void playSequence(const ToneNote *notes, size_t len);

    /** Воспроизвести сырой PCM (8-bit unsigned, 8000 Hz) через DAC. */
    void playRawPcm(const uint8_t *data, size_t len,
                    uint32_t sample_rate_hz = 8000);

    // Удобные методы для конкретных звуков
    void cardFound()     { playSequence(SOUND_CARD_FOUND,    SOUND_CARD_FOUND_LEN);    }
    void cardWritten()   { playSequence(SOUND_CARD_WRITTEN,  SOUND_CARD_WRITTEN_LEN);  }
    void taskPrompt()    { playSequence(SOUND_TASK_PROMPT,   SOUND_TASK_PROMPT_LEN);   }
    void taskDone()      { playSequence(SOUND_TASK_DONE,     SOUND_TASK_DONE_LEN);     }
    void error()         { playSequence(SOUND_ERROR,         SOUND_ERROR_LEN);         }
    void secondTouch()   { playSequence(SOUND_SECOND_TOUCH,  SOUND_SECOND_TOUCH_LEN);  }

private:
    void setFreq(gpio_num_t gpio, ledc_channel_t channel, uint32_t freq_hz);
    void stopChannel(ledc_channel_t channel);

    gpio_num_t      speaker_gpio_;
    gpio_num_t      buzzer_gpio_;
    bool            inited_ = false;
};

} // namespace audio
