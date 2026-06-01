#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "driver/gptimer.h"
#include "esp_log.h"

#define TX_GPIO_NUM          13
#define CARRIER_FREQ_HZ      175000   // Частота несущей 175 кГц
#define BURST_DURATION_US    100      // Длительность пачки 100 мкс
#define TOTAL_PERIOD_US      50000    // Период повторения 50 мс (50000 мкс)

// Расчет заполнения ШИМ для 10-битного разрешения (2^10 = 1024)
// 50% заполнение = 512. 0% (выключено) = 0.
#define LEDC_DUTY_50_PERCENT 128
#define LEDC_DUTY_OFF        0

static const char *TAG = "PWM_BURST";
static gptimer_handle_t gptimer = NULL;

// Прерывание таймера: вызывается строго каждые 50 мс
static bool IRAM_ATTR timer_on_alarm_cb(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx)
{
    // 1. Мгновенно включаем генерацию 175 кГц (скважность 50%)
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, LEDC_DUTY_50_PERCENT);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);

    // 2. Аппаратная задержка внутри прерывания ровно на 100 мкс
    // Для ультра-коротких пачек в 100 мкс esp_rom_delay_us идеален — он точен до наносекунд
    esp_rom_delay_us(BURST_DURATION_US);

    // 3. Выключаем генерацию (переводим пин в стабильный 0)
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, LEDC_DUTY_OFF);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);

    return true; // Контекст FreeRTOS не меняется
}

void app_main(void)
{
    ESP_LOGI(TAG, "Configuring LEDC (PWM) at 175 kHz...");

    // 1. Настройка таймера ШИМ на 175 кГц
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .timer_num        = LEDC_TIMER_0,
        .duty_resolution  = LEDC_TIMER_8_BIT, // 10 бит разрешения дает высокую точность на 175 кГц
        .freq_hz          = CARRIER_FREQ_HZ,  // Жестко 175000 Гц
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // 2. Настройка канала ШИМ на GPIO 13
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = LEDC_CHANNEL_0,
        .timer_sel      = LEDC_TIMER_0,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = TX_GPIO_NUM,
        .duty           = LEDC_DUTY_OFF, // Изначально генерация выключена
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

    ESP_LOGI(TAG, "Configuring Hardware GPTimer for 50ms period...");

    // 3. Инициализация аппаратного таймера
    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1000000, // 1 МГц = 1 тик равен 1 мкс
    };
    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &gptimer));

    // Настройка периода в 50 мс (50 000 мкс)
    gptimer_alarm_config_t alarm_config = {
        .reload_count = 0,
        .alarm_count = TOTAL_PERIOD_US, 
        .flags.auto_reload_on_alarm = true
    };
    ESP_ERROR_CHECK(gptimer_set_alarm_action(gptimer, &alarm_config));

    // Подключаем функцию прерывания
    gptimer_event_callbacks_t cbs = {
        .on_alarm = timer_on_alarm_cb,
    };
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(gptimer, &cbs, NULL));

    // Включаем и запускаем таймер
    ESP_ERROR_CHECK(gptimer_enable(gptimer));
    ESP_ERROR_CHECK(gptimer_start(gptimer));

    ESP_LOGI(TAG, "System stable. Burst generation running in background via Hardware Interrupts.");

    while (1) {
        // Процессор полностью свободен, Watchdog молчит, память стабильна
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
