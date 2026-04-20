#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <sys/time.h>

#include "config/manager.h"
#include "config/wifi_portal.h"
#include "checkpoint/logic.h"

static const char *TAG = "Main";

// SPI пины (общая шина для RC522 и SX1276)
static constexpr int GPIO_SPI_SCK  = 15;
static constexpr int GPIO_SPI_MOSI = 2;
static constexpr int GPIO_SPI_MISO = 4;

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "FoxAss Checkpoint starting...");

    // ——— 1. NVS + Конфигурация ——————————————————————————————————————————
    config::ConfigManager cfg_mgr;
    ESP_ERROR_CHECK(cfg_mgr.init());

    const config::StationConfig &cfg = cfg_mgr.get();
    ESP_LOGI(TAG, "Config loaded: cp_num=%d valid=%d",
             cfg.cp_num, cfg.isValid());

    // ——— 2. SPI шина ————————————————————————————————————————————————————
    spi_bus_config_t buscfg = {};
    buscfg.mosi_io_num     = GPIO_SPI_MOSI;
    buscfg.miso_io_num     = GPIO_SPI_MISO;
    buscfg.sclk_io_num     = GPIO_SPI_SCK;
    buscfg.quadwp_io_num   = -1;
    buscfg.quadhd_io_num   = -1;
    buscfg.max_transfer_sz = 512;

    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));
    ESP_LOGI(TAG, "SPI bus initialized");

    // ——— 3. Checkpoint logic (создаём объект, init — после портала) ————————
    checkpoint::CheckpointLogic *cp =
        new checkpoint::CheckpointLogic(SPI2_HOST, cfg_mgr);

    // Инициализировать LED до портала — показываем красный пульс "ожидание настройки"
    cp->initLed();

    // ——— 4. WiFi portal ——————————————————————————————————————————————————
    // Без валидного конфига — ждём бесконечно.
    // С валидным конфигом — ждём 30 сек; при подключении клиента — бесконечно.
    config::WiFiPortal portal(cfg_mgr);

    volatile bool start_requested = false;

    // portal.stop() нельзя вызывать из колбэка — он выполняется внутри httpd
    // handler'а, а httpd_stop() ждёт завершения handler'а → дедлок.
    // Ставим только флаг, stop() вызываем из main task ниже.
    portal.setStartCallback([&]() {
        start_requested = true;
    });

    uint64_t portal_start_us = esp_timer_get_time();
    ESP_ERROR_CHECK(portal.start());

    // ——— 5. Ждём команды старта или таймаута ————————————————————————————
    ESP_LOGI(TAG, "Waiting for /api/start or portal timeout...");

    while (!start_requested && portal.isRunning()) {
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    // Останавливаем portal из main task (не из httpd handler'а)
    portal.stop();

    // ——— 5а. Восстановление времени из NVS при таймауте ——————————————————
    // Если клиент не подключился за 30 сек — применяем последний сохранённый
    // timestamp + время фактического ожидания (≈30 сек).
    if (portal.timedOut()) {
        uint32_t last_ts = cfg_mgr.get().last_timestamp;
        if (last_ts > 0) {
            uint32_t elapsed_sec = (uint32_t)
                ((esp_timer_get_time() - portal_start_us) / 1000000ULL);
            uint32_t corrected_ts = last_ts + elapsed_sec;
            struct timeval tv = { .tv_sec = (time_t)corrected_ts, .tv_usec = 0 };
            settimeofday(&tv, nullptr);
            ESP_LOGI(TAG, "NVS timestamp restored: %lu + %lus = %lu",
                     (unsigned long)last_ts,
                     (unsigned long)elapsed_sec,
                     (unsigned long)corrected_ts);
        } else {
            ESP_LOGW(TAG, "Portal timed out but no saved timestamp in NVS");
        }
    }

    ESP_LOGI(TAG, "WiFi portal done, starting checkpoint operation");

    // ——— 6. Инициализировать checkpoint с актуальным конфигом из NVS ———————
    // Всегда после портала — гарантирует свежие station_key/sector_key
    if (!cfg_mgr.get().isValid()) {
        ESP_LOGE(TAG, "No valid config! Reboot or reconfigure.");
        while (true) vTaskDelay(pdMS_TO_TICKS(5000));
    }

    {
        esp_err_t ret = cp->init();
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Checkpoint init failed: %s", esp_err_to_name(ret));
            while (true) vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    // ——— 7. Запустить задачи КП ——————————————————————————————————————————
    cp->start();

    ESP_LOGI(TAG, "Checkpoint running. cp_num=%d is_fox=%d",
             cfg_mgr.get().cp_num, cfg_mgr.get().is_fox);

    // Главная задача завершается — FreeRTOS задачи продолжают работу
    vTaskDelete(nullptr);
}
