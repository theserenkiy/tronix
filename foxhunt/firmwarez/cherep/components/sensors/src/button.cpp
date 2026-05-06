#include "sensors/button.h"
#include "esp_log.h"

static const char *TAG = "Button";

namespace sensors {

Button::Button(gpio_num_t gpio, uint32_t long_press_ms)
    : gpio_(gpio), long_press_ms_(long_press_ms)
{}

Button::~Button()
{
    gpio_isr_handler_remove(gpio_);
    if (debounce_timer_)   xTimerDelete(debounce_timer_,   0);
    if (long_press_timer_) xTimerDelete(long_press_timer_, 0);
}

esp_err_t Button::init()
{
    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << gpio_);
    io_conf.mode         = GPIO_MODE_INPUT;
    io_conf.pull_up_en   = GPIO_PULLUP_ENABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type    = GPIO_INTR_ANYEDGE;

    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) return ret;

    debounce_timer_ = xTimerCreate("btn_debounce", pdMS_TO_TICKS(20),
                                   pdFALSE, this, debounceCallback);
    long_press_timer_ = xTimerCreate("btn_long", pdMS_TO_TICKS(long_press_ms_),
                                     pdFALSE, this, longPressCallback);

    gpio_install_isr_service(0); // OK если уже установлен
    gpio_isr_handler_add(gpio_, isrHandler, this);

    ESP_LOGI(TAG, "Button init on GPIO%d", gpio_);
    return ESP_OK;
}

bool Button::isPressed() const
{
    return gpio_get_level(gpio_) == 0;
}

void IRAM_ATTR Button::isrHandler(void *arg)
{
    Button *self = static_cast<Button *>(arg);
    // Запустить дебоунс таймер (отложенная обработка)
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xTimerResetFromISR(self->debounce_timer_, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void Button::debounceCallback(TimerHandle_t timer)
{
    Button *self = static_cast<Button *>(pvTimerGetTimerID(timer));
    self->handleDebounced();
}

void Button::longPressCallback(TimerHandle_t timer)
{
    Button *self = static_cast<Button *>(pvTimerGetTimerID(timer));
    if (self->isPressed()) {
        self->long_fired_ = true;
        ESP_LOGD(TAG, "Long press fired");
        if (self->long_press_cb_) self->long_press_cb_();
    }
}

void Button::handleDebounced()
{
    bool currently_pressed = isPressed();

    if (currently_pressed && !pressed_) {
        // Нажатие
        pressed_    = true;
        long_fired_ = false;
        xTimerReset(long_press_timer_, 0);
        if (change_cb_) change_cb_();
        ESP_LOGD(TAG, "Pressed");

    } else if (!currently_pressed && pressed_) {
        // Отпускание
        pressed_ = false;
        xTimerStop(long_press_timer_, 0);

        if (!long_fired_) {
            // Короткое нажатие
            ESP_LOGD(TAG, "Short press");
            if (short_press_cb_) short_press_cb_();
        }
        if (change_cb_) change_cb_();
    }
}

} // namespace sensors
