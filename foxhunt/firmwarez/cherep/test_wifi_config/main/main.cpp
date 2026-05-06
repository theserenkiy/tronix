/**
 * FoxAss CP Mock — имитация WiFi-портала контрольного пункта.
 *
 * Назначение: тестирование веб-морды конфигурирования КП без реального железа.
 * Запускает WiFi AP, отвечает на все API запросы (GET/POST /api/config,
 * POST /api/sync_time, GET /api/status, POST /api/start).
 *
 * После POST /api/start WiFi НЕ отключается — это мок, нам нужно видеть
 * результат в браузере. Состояние меняется на RUNNING, что отражается в
 * GET /api/status (поле "mock_state").
 *
 * LED на GPIO2 (встроенный на ESP32 DevKit v1):
 *   - Медленное мигание 1 Гц  = портал активен, конфиг НЕ записан
 *   - Быстрое мигание 4 Гц    = портал активен, конфиг записан
 *   - Двойное мигание раз в сек = получена команда /api/start (RUNNING)
 */

#include "config/manager.h"
#include "config/wifi_portal.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#include <sys/time.h>
#include <cinttypes>

static const char *TAG = "MockCP";

// Встроенный LED на DevKit v1
static constexpr gpio_num_t GPIO_LED = GPIO_NUM_2;

// ——— Состояние мока —————————————————————————————————————————————————————————

enum class MockState : uint8_t {
    PORTAL_ACTIVE = 0,  // WiFi поднят, ждём конфигурации или команды старт
    RUNNING       = 1,  // Получена команда /api/start
};

static volatile MockState s_state = MockState::PORTAL_ACTIVE;

// Глобальные объекты (статические — чтобы жили всё время работы)
static config::ConfigManager s_cfg_mgr;
static config::WiFiPortal   *s_portal = nullptr;

// ——— LED задача ——————————————————————————————————————————————————————————————

static void led_task(void * /*arg*/)
{
    gpio_config_t io = {};
    io.pin_bit_mask = (1ULL << GPIO_LED);
    io.mode         = GPIO_MODE_OUTPUT;
    io.pull_up_en   = GPIO_PULLUP_DISABLE;
    io.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io.intr_type    = GPIO_INTR_DISABLE;
    gpio_config(&io);

    while (true) {
        switch (s_state) {
        case MockState::PORTAL_ACTIVE:
            if (s_cfg_mgr.get().initialized) {
                // Быстрое мигание 4 Гц: конфиг есть, ждём команды старт
                gpio_set_level(GPIO_LED, 1);
                vTaskDelay(pdMS_TO_TICKS(125));
                gpio_set_level(GPIO_LED, 0);
                vTaskDelay(pdMS_TO_TICKS(125));
            } else {
                // Медленное мигание 1 Гц: конфиг не записан
                gpio_set_level(GPIO_LED, 1);
                vTaskDelay(pdMS_TO_TICKS(500));
                gpio_set_level(GPIO_LED, 0);
                vTaskDelay(pdMS_TO_TICKS(500));
            }
            break;

        case MockState::RUNNING:
            // Двойное мигание: КП "запущен"
            gpio_set_level(GPIO_LED, 1);
            vTaskDelay(pdMS_TO_TICKS(80));
            gpio_set_level(GPIO_LED, 0);
            vTaskDelay(pdMS_TO_TICKS(80));
            gpio_set_level(GPIO_LED, 1);
            vTaskDelay(pdMS_TO_TICKS(80));
            gpio_set_level(GPIO_LED, 0);
            vTaskDelay(pdMS_TO_TICKS(760));
            break;
        }
    }
}

// ——— Дополнительный handler: GET /api/mock/state ————————————————————————————
// Возвращает мок-специфичный статус: текущее состояние, время, конфиг.

static esp_err_t handle_mock_state(httpd_req_t *req)
{
    const config::StationConfig &cfg = s_cfg_mgr.get();

    struct timeval tv;
    gettimeofday(&tv, nullptr);

    char hex_station_key[33] = {};
    for (int i = 0; i < 16; i++) {
        snprintf(hex_station_key + i * 2, 3, "%02x", cfg.station_key[i]);
    }
    char hex_sector_key[13] = {};
    for (int i = 0; i < 6; i++) {
        snprintf(hex_sector_key + i * 2, 3, "%02x", cfg.sector_key[i]);
    }

    const char *state_str = (s_state == MockState::RUNNING) ? "running" : "portal_active";

    char json[512];
    snprintf(json, sizeof(json),
        "{"
        "\"mock\":true,"
        "\"mock_state\":\"%s\","
        "\"unix_time\":%" PRId64 ","
        "\"uptime_sec\":%" PRIu32 ","
        "\"cp_num\":%d,"
        "\"event_id\":%d,"
        "\"station_key\":\"%s\","
        "\"sector_key\":\"%s\","
        "\"initialized\":%s,"
        "\"has_task\":%s,"
        "\"is_fox\":%s"
        "}",
        state_str,
        (int64_t)tv.tv_sec,
        (uint32_t)(esp_timer_get_time() / 1000000ULL),
        cfg.cp_num, cfg.event_id,
        hex_station_key, hex_sector_key,
        cfg.initialized ? "true" : "false",
        cfg.has_task    ? "true" : "false",
        cfg.is_fox      ? "true" : "false");

    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, json);
    return ESP_OK;
}

// ——— Регистрация дополнительных handlers через httpd handle —————————————————

static void register_mock_handlers(httpd_handle_t httpd)
{
    httpd_uri_t uri = {
        .uri      = "/api/mock/state",
        .method   = HTTP_GET,
        .handler  = handle_mock_state,
        .user_ctx = nullptr,
    };
    httpd_register_uri_handler(httpd, &uri);
}

// ——— app_main ————————————————————————————————————————————————————————————————

extern "C" void app_main()
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  FoxAss CP Mock — WiFi Config Tester  ");
    ESP_LOGI(TAG, "========================================");

    // Инициализация конфига (NVS)
    esp_err_t ret = s_cfg_mgr.init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ConfigManager init failed: %s", esp_err_to_name(ret));
        return;
    }

    const config::StationConfig &cfg = s_cfg_mgr.get();
    ESP_LOGI(TAG, "Config loaded: cp_num=0x%02X event_id=%d initialized=%d",
             cfg.cp_num, cfg.event_id, (int)cfg.initialized);

    // Создаём портал
    s_portal = new config::WiFiPortal(s_cfg_mgr);

    // Callback на команду /api/start
    s_portal->setStartCallback([]() {
        ESP_LOGI(TAG, ">>> /api/start received — mock transitions to RUNNING");
        ESP_LOGI(TAG, ">>> (WiFi остаётся активным для тестирования веб-морды)");
        s_state = MockState::RUNNING;
        // В реальном КП здесь: s_portal->stop() + запуск checkpoint logic.
        // В моке намеренно НЕ вызываем stop() — WiFi должен оставаться.
    });

    // Запуск портала без таймаута (мок должен быть доступен всегда)
    ret = s_portal->start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "WiFi portal start failed: %s", esp_err_to_name(ret));
        return;
    }

    // Регистрируем мок-специфичный эндпоинт /api/mock/state
    // WiFiPortal не предоставляет прямой доступ к httpd_handle — регистрируем
    // через отдельный httpd порт 81 чтобы не конфликтовать.
    {
        httpd_config_t httpd_cfg = HTTPD_DEFAULT_CONFIG();
        httpd_cfg.server_port = 81;
        httpd_cfg.stack_size  = 4096;
        httpd_handle_t mock_httpd = nullptr;
        if (httpd_start(&mock_httpd, &httpd_cfg) == ESP_OK) {
            register_mock_handlers(mock_httpd);
            ESP_LOGI(TAG, "Mock debug endpoint: GET http://192.168.4.1:81/api/mock/state");
        }
    }

    // Запуск LED задачи
    xTaskCreate(led_task, "led_mock", 2048, nullptr, 2, nullptr);

    ESP_LOGI(TAG, "----------------------------------------");
    ESP_LOGI(TAG, "WiFi AP: FoxAss-CP-%02X (open)", cfg.cp_num);
    ESP_LOGI(TAG, "IP: 192.168.4.1");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Эндпоинты (порт 80):");
    ESP_LOGI(TAG, "  GET  http://192.168.4.1/api/config");
    ESP_LOGI(TAG, "  POST http://192.168.4.1/api/config");
    ESP_LOGI(TAG, "  POST http://192.168.4.1/api/sync_time");
    ESP_LOGI(TAG, "  GET  http://192.168.4.1/api/status");
    ESP_LOGI(TAG, "  POST http://192.168.4.1/api/start");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Мок-отладка (порт 81):");
    ESP_LOGI(TAG, "  GET  http://192.168.4.1:81/api/mock/state");
    ESP_LOGI(TAG, "----------------------------------------");
    ESP_LOGI(TAG, "LED на GPIO2:");
    ESP_LOGI(TAG, "  Медленно 1Гц  = ждём первичной настройки");
    ESP_LOGI(TAG, "  Быстро 4Гц    = конфиг записан, ждём /api/start");
    ESP_LOGI(TAG, "  Двойное 1/сек = получена команда start (RUNNING)");
    ESP_LOGI(TAG, "----------------------------------------");

    // Главный цикл: логировать состояние раз в минуту
    uint32_t tick = 0;
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(60000));
        tick++;

        struct timeval tv;
        gettimeofday(&tv, nullptr);

        ESP_LOGI(TAG, "[%lu мин] state=%s cp=0x%02X time=%" PRId64 " portal=%s",
                 (unsigned long)tick,
                 s_state == MockState::RUNNING ? "RUNNING" : "PORTAL",
                 s_cfg_mgr.get().cp_num,
                 (int64_t)tv.tv_sec,
                 s_portal->isRunning() ? "up" : "down");
    }
}
