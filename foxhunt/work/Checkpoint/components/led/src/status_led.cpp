#include "led/status_led.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "StatusLed";

namespace led {

StatusLed::StatusLed(gpio_num_t gpio) : gpio_(gpio) {}

StatusLed::~StatusLed()
{
    stop_anim_ = true;
    if (anim_task_) {
        vTaskDelay(pdMS_TO_TICKS(200));
        if (anim_task_) vTaskDelete(anim_task_);
    }
    if (strip_) led_strip_del(strip_);
}

esp_err_t StatusLed::init()
{
    led_strip_config_t strip_config = {
        .strip_gpio_num   = gpio_,
        .max_leds         = 1,
        .led_model        = LED_MODEL_WS2812,
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
        .flags = {
            .invert_out = false,
        },
    };

    led_strip_rmt_config_t rmt_config = {
        .clk_src        = RMT_CLK_SRC_DEFAULT,
        .resolution_hz  = 10 * 1000 * 1000, // 10 МГц
        .mem_block_symbols = 64,
        .flags = {
            .with_dma = false,
        },
    };

    esp_err_t ret = led_strip_new_rmt_device(&strip_config, &rmt_config, &strip_);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LED strip init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    off();
    ESP_LOGI(TAG, "WS2812 LED init on GPIO%d", gpio_);
    return ESP_OK;
}

// ——— Прямое управление ——————————————————————————————————————————————————

void StatusLed::rawSet(uint8_t r, uint8_t g, uint8_t b)
{
    if (!strip_) return;
    led_strip_set_pixel(strip_, 0, r, g, b);
    led_strip_refresh(strip_);
}

void StatusLed::setColor(uint8_t r, uint8_t g, uint8_t b)
{
    stop_anim_ = true;
    if (anim_task_) {
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    rawSet(r, g, b);
}

void StatusLed::off()
{
    stop_anim_ = true;
    if (strip_) {
        led_strip_clear(strip_);
        led_strip_refresh(strip_);
    }
}

// ——— Анимации ————————————————————————————————————————————————————————————

void StatusLed::blink(uint8_t r, uint8_t g, uint8_t b,
                       uint32_t on_ms, uint32_t off_ms)
{
    while (!stop_anim_) {
        rawSet(r, g, b);
        for (uint32_t i = 0; i < on_ms / 10 && !stop_anim_; i++)
            vTaskDelay(pdMS_TO_TICKS(10));

        rawSet(0, 0, 0);
        for (uint32_t i = 0; i < off_ms / 10 && !stop_anim_; i++)
            vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void StatusLed::doubleBlink(uint8_t r, uint8_t g, uint8_t b,
                             uint32_t on_ms, uint32_t off_ms, uint32_t pause_ms)
{
    while (!stop_anim_) {
        // Первый мигок
        rawSet(r, g, b);
        for (uint32_t i = 0; i < on_ms / 10 && !stop_anim_; i++)
            vTaskDelay(pdMS_TO_TICKS(10));
        rawSet(0, 0, 0);
        for (uint32_t i = 0; i < off_ms / 10 && !stop_anim_; i++)
            vTaskDelay(pdMS_TO_TICKS(10));
        // Второй мигок
        rawSet(r, g, b);
        for (uint32_t i = 0; i < on_ms / 10 && !stop_anim_; i++)
            vTaskDelay(pdMS_TO_TICKS(10));
        rawSet(0, 0, 0);
        // Пауза после пары
        for (uint32_t i = 0; i < pause_ms / 10 && !stop_anim_; i++)
            vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void StatusLed::pulse(uint8_t r, uint8_t g, uint8_t b)
{
    const int steps = 20;
    while (!stop_anim_) {
        for (int i = 0; i <= steps && !stop_anim_; i++) {
            float t = (float)i / steps;
            rawSet((uint8_t)(r * t), (uint8_t)(g * t), (uint8_t)(b * t));
            vTaskDelay(pdMS_TO_TICKS(20));
        }
        for (int i = steps; i >= 0 && !stop_anim_; i--) {
            float t = (float)i / steps;
            rawSet((uint8_t)(r * t), (uint8_t)(g * t), (uint8_t)(b * t));
            vTaskDelay(pdMS_TO_TICKS(20));
        }
    }
}

// ——— animTask ————————————————————————————————————————————————————————————

void StatusLed::animTask(void *arg)
{
    StatusLed *self = static_cast<StatusLed *>(arg);
    self->runAnimation();
    self->anim_task_ = nullptr;
    vTaskDelete(nullptr);
}

void StatusLed::runAnimation()
{
    switch (current_status_) {
    case LedStatus::IDLE:
        blink(0, 40, 0, 500, 1500);          // зелёный медленно — КП готов
        break;
    case LedStatus::WIFI_CONFIG:
        pulse(80, 0, 0);                     // красный плавно — запуск/настройка
        break;
    case LedStatus::CARD_FOUND:
        rawSet(80, 80, 80);                  // белый постоянный — карта найдена
        while (!stop_anim_) vTaskDelay(pdMS_TO_TICKS(100));
        break;
    case LedStatus::WAITING_TASK:
        blink(60, 60, 0, 200, 200);          // жёлтый быстро — переходной
        break;
    case LedStatus::TASK_IN_PROGRESS:
        pulse(80, 15, 0);                    // оранжево-красный плавно — прогрев
        break;
    case LedStatus::ALCO_BLOW:
        blink(0, 60, 0, 200, 200);           // зелёный быстро — дуть!
        break;
    case LedStatus::TASK_DONE:
        doubleBlink(0, 0, 80, 150, 150, 700); // двойной синий — ждём карту
        break;
    case LedStatus::ERROR:
        blink(80, 0, 0, 100, 100);           // красный быстро — ошибка
        break;
    case LedStatus::FOX_MODE:
        blink(60, 0, 80, 300, 700);          // фиолетовый мигает — лиса
        break;
    case LedStatus::OFF:
    default:
        rawSet(0, 0, 0);
        break;
    }
}

// ——— setStatus ———————————————————————————————————————————————————————————

void StatusLed::setStatus(LedStatus status)
{
    if (current_status_ == status) return;

    // Остановить текущую анимацию
    stop_anim_ = true;
    if (anim_task_) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    current_status_ = status;
    stop_anim_      = false;

    // Запустить новую анимацию в отдельной задаче
    xTaskCreate(animTask, "led_anim", 2048, this, 1, &anim_task_);
}

} // namespace led
