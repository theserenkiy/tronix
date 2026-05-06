#pragma once

#include "driver/gpio.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include <functional>
#include <cstdint>

namespace sensors {

/**
 * Кнопка с дебоунсом и поддержкой коротких/длинных нажатий.
 *
 * GPIO32, активный уровень LOW (нажата = 0).
 * Дебоунс: 20 мс.
 * Длинное нажатие: настраиваемо (по умолчанию 1000 мс).
 */
class Button {
public:
    using Callback = std::function<void()>;

    /**
     * @param gpio         GPIO кнопки
     * @param long_press_ms Порог длинного нажатия в мс
     */
    explicit Button(gpio_num_t gpio, uint32_t long_press_ms = 1000);
    ~Button();

    /** Инициализировать GPIO и ISR. */
    esp_err_t init();

    /** Установить callback на короткое нажатие. */
    void onShortPress(Callback cb)    { short_press_cb_ = cb; }

    /** Установить callback на длинное нажатие. */
    void onLongPress(Callback cb)     { long_press_cb_ = cb; }

    /** Установить callback на любое нажатие (для опроса). */
    void onChange(Callback cb)        { change_cb_ = cb; }

    /** Текущее состояние (true = нажата). */
    bool isPressed() const;

private:
    static void IRAM_ATTR isrHandler(void *arg);
    static void debounceCallback(TimerHandle_t timer);
    static void longPressCallback(TimerHandle_t timer);

    void handleDebounced();

    gpio_num_t   gpio_;
    uint32_t     long_press_ms_;
    Callback     short_press_cb_;
    Callback     long_press_cb_;
    Callback     change_cb_;
    TimerHandle_t debounce_timer_  = nullptr;
    TimerHandle_t long_press_timer_= nullptr;
    volatile bool pressed_         = false;
    volatile bool long_fired_      = false;
};

} // namespace sensors
