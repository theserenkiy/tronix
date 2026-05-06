#pragma once

#include "driver/gpio.h"
#include "led_strip.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cstdint>

namespace led {

/** Статусы светодиода. */
enum class LedStatus {
    OFF,
    IDLE,             // Зелёный мигает медленно — обычный КП готов
    WIFI_CONFIG,      // Красный пульсирует — запуск / ожидание настройки
    CARD_FOUND,       // Белый постоянный — карта в поле
    WAITING_TASK,     // Жёлтый мигает быстро — переходной
    TASK_IN_PROGRESS, // Красный пульсирует — прогрев нагревателя
    ALCO_BLOW,        // Зелёный мигает — дуть в датчик
    TASK_DONE,        // Двойное синее мигание — ждём второго касания
    ERROR,            // Красный мигает быстро — ошибка
    FOX_MODE,         // Фиолетовый мигает — режим лисы
};

/**
 * Управление одиночным WS2812B светодиодом.
 * Использует ESP-IDF led_strip API через RMT.
 * Поддерживает анимации (мигание, пульсация) через отдельную задачу.
 */
class StatusLed {
public:
    /**
     * @param gpio    GPIO данных WS2812B (G33)
     */
    explicit StatusLed(gpio_num_t gpio);
    ~StatusLed();

    /** Инициализировать LED strip. */
    esp_err_t init();

    /** Установить постоянный цвет. */
    void setColor(uint8_t r, uint8_t g, uint8_t b);

    /** Выключить. */
    void off();

    /** Установить статус (запускает соответствующую анимацию). */
    void setStatus(LedStatus status);

    LedStatus getStatus() const { return current_status_; }

private:
    static void animTask(void *arg);
    void        runAnimation();

    void        rawSet(uint8_t r, uint8_t g, uint8_t b);
    void        blink(uint8_t r, uint8_t g, uint8_t b,
                      uint32_t on_ms, uint32_t off_ms);
    void        doubleBlink(uint8_t r, uint8_t g, uint8_t b,
                            uint32_t on_ms, uint32_t off_ms, uint32_t pause_ms);
    void        pulse(uint8_t r, uint8_t g, uint8_t b);

    gpio_num_t         gpio_;
    led_strip_handle_t strip_     = nullptr;
    LedStatus          current_status_ = LedStatus::OFF;
    TaskHandle_t       anim_task_ = nullptr;
    volatile bool      stop_anim_ = false;
};

} // namespace led
