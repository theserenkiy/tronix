/**
 * FoxAss MQ-3 Test
 *
 * Фаза 1 — тест:
 *   - 20 сек прогрева (heater ON)
 *   - Приглашение дуть
 *   - 5 сек замеров (каждые 100ms) при включённом нагревателе
 *   - Вывод среднего, мин, макс
 *
 * Фаза 2 — длительный прогрев:
 *   - Heater включён постоянно, каждые 10 сек — сырое значение ADC в лог
 *   - Работает до сброса
 *
 * Пины: G27 — heater (active HIGH), G25 — ADC2_CH8
 */

#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "MQ3Test";

static constexpr gpio_num_t GPIO_HEATER  = GPIO_NUM_27;
static constexpr adc_channel_t ADC_CH    = ADC_CHANNEL_8;  // GPIO25 = ADC2_CH8
static constexpr adc_unit_t    ADC_UNIT  = ADC_UNIT_2;

// ——— Утилиты ————————————————————————————————————————————————————————————————

static adc_oneshot_unit_handle_t adc_handle;

static void heater_set(bool on)
{
    gpio_set_level(GPIO_HEATER, on ? 1 : 0);
    ESP_LOGI(TAG, "Heater %s", on ? "ON" : "OFF");
}

static int adc_read()
{
    int raw = 0;
    adc_oneshot_read(adc_handle, ADC_CH, &raw);
    return raw;
}

// Обратный отсчёт с логом каждую секунду
static void countdown(const char *label, int seconds)
{
    for (int i = seconds; i > 0; i--) {
        ESP_LOGI(TAG, "%s: %d сек...", label, i);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// ——— Фаза 1: тест ——————————————————————————————————————————————————————————

static void phase1_test()
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  Фаза 1: Тест алкотестера");
    ESP_LOGI(TAG, "========================================");

    // Прогрев 20 секунд
    heater_set(true);
    ESP_LOGI(TAG, "Прогрев датчика...");
    countdown("Прогрев", 20);

    // Приглашение
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, ">>> ДУЙТЕ В ДАТЧИК <<<");
    ESP_LOGI(TAG, "");

    // 5 секунд замеров (heater не выключаем)
    const int SAMPLES   = 50;           // 50 * 100ms = 5 сек
    const int INTERVAL  = 100;          // ms
    int sum = 0, mn = 4095, mx = 0;

    for (int i = 0; i < SAMPLES; i++) {
        int v = adc_read();
        sum += v;
        if (v < mn) mn = v;
        if (v > mx) mx = v;
        ESP_LOGI(TAG, "  [%2d/%d] raw=%d", i + 1, SAMPLES, v);
        vTaskDelay(pdMS_TO_TICKS(INTERVAL));
    }

    int avg = sum / SAMPLES;
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "=== Результат теста ===");
    ESP_LOGI(TAG, "  Среднее : %d", avg);
    ESP_LOGI(TAG, "  Мин     : %d", mn);
    ESP_LOGI(TAG, "  Макс    : %d", mx);
    ESP_LOGI(TAG, "  (4095 = 3.3V, 0 = 0V; чистый воздух ~400-600, алкоголь выше)");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Фаза 1 завершена. Переходим к фазе 2 (длительный прогрев).");
    ESP_LOGI(TAG, "");
}

// ——— Фаза 2: длительный прогрев ————————————————————————————————————————————

static void phase2_burnin()
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  Фаза 2: Длительный прогрев (24ч)");
    ESP_LOGI(TAG, "  Heater постоянно включён.");
    ESP_LOGI(TAG, "  Сброс платы для остановки.");
    ESP_LOGI(TAG, "========================================");

    heater_set(true);   // уже включён, но явно

    uint32_t elapsed_sec = 0;
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(10000));
        elapsed_sec += 10;

        int raw = adc_read();
        uint32_t h = elapsed_sec / 3600;
        uint32_t m = (elapsed_sec % 3600) / 60;
        uint32_t s = elapsed_sec % 60;
        ESP_LOGI(TAG, "[%02lu:%02lu:%02lu] raw=%d", h, m, s, raw);
    }
}

// ——— app_main ————————————————————————————————————————————————————————————————

extern "C" void app_main()
{
    ESP_LOGI(TAG, "FoxAss MQ-3 Test start");

    // GPIO heater
    gpio_config_t hcfg = {};
    hcfg.pin_bit_mask = (1ULL << GPIO_HEATER);
    hcfg.mode         = GPIO_MODE_OUTPUT;
    hcfg.intr_type    = GPIO_INTR_DISABLE;
    gpio_config(&hcfg);
    gpio_set_level(GPIO_HEATER, 0);

    // ADC2 oneshot
    adc_oneshot_unit_init_cfg_t unit_cfg = {};
    unit_cfg.unit_id  = ADC_UNIT;
    unit_cfg.ulp_mode = ADC_ULP_MODE_DISABLE;
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_cfg, &adc_handle));

    adc_oneshot_chan_cfg_t chan_cfg = {};
    chan_cfg.bitwidth = ADC_BITWIDTH_12;
    chan_cfg.atten    = ADC_ATTEN_DB_12;   // 0–3.3V full range
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CH, &chan_cfg));

    // Тест, затем длительный прогрев
    phase1_test();
    phase2_burnin();
}
