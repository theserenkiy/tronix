#pragma once

#include "lora/sx1276.h"
#include "config/types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cstdint>

namespace lora {

/**
 * Режим «лисы» (ARDF Fox) — FSK с аудио тоном.
 *
 * Алгоритм:
 * 1. Переключить SX1276 в FSK TX continuous режим
 * 2. Передать Morse-строку тоном 800 Гц через DIO2 (AFSK)
 *    - Dit/Dah: меандр 800 Гц на DIO2 → FM-приёмник слышит тон
 *    - Паузы: DIO2=0 → тишина на приёмнике
 * 3. Выдержать паузу fox_off_sec секунд
 * 4. Вернуться в LoRa RX режим
 *
 * Параметры эфира:
 *   Девиация:  ±3500 Гц (NFM стандарт ±5 кГц, хороший уровень звука)
 *   Тон CW:    800 Гц (оптимально для слышимости на Baofeng через помехи)
 *
 * Тайминги Морзе (ARDF стандарт):
 *  - dit  = 50 мс, dah = 150 мс
 *  - пауза внутри символа = 50 мс
 *  - пауза между символами = 150 мс
 *  - пауза между словами = 350 мс
 */
class FoxMode {
public:

    /**
     * @param radio    SX1276 — должен быть инициализирован в LoRa режиме
     * @param dio2_gpio GPIO для DIO2 (FSK прямая модуляция)
     * @param cfg      Конфиг станции
     */
    FoxMode(SX1276 &radio, gpio_num_t dio2_gpio, const config::StationConfig &cfg);

    /** Запустить FreeRTOS задачу Fox. */
    void start();

    /** Остановить задачу Fox. */
    void stop();

    bool isRunning() const { return task_handle_ != nullptr; }

private:
    static void foxTask(void *arg);
    void        runCycle();

    // Передача одного Morse символа
    void sendMorseChar(char c);
    void sendMorseString(const char *str);
    void dit();
    void dah();
    void symbolGap();
    void letterGap();
    void wordGap();

    /** Генерировать тон TONE_HZ на DIO2 в течение duration_ms мс (меандр). */
    void toneBurst(uint32_t duration_ms);
    /** Тишина: DIO2=0, пауза duration_ms мс. */
    void silence(uint32_t duration_ms);

    void switchToFSK();
    void switchToLoRa();

    SX1276                     &radio_;
    gpio_num_t                  dio2_gpio_;
    const config::StationConfig &cfg_;
    TaskHandle_t                task_handle_ = nullptr;
    volatile bool               stop_flag_   = false;

    uint32_t fox_freq_hz_  = 433920000;
    uint32_t fox_fdev_hz_  = 3500;
    uint32_t fox_tone_hz_  = 800;
    uint32_t lora_freq_hz_ = 433175000;
};

} // namespace lora
