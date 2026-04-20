/**
 * FoxAss LoRa FSK Test — непрерывная передача морзянки в FSK режиме.
 *
 * Назначение: проверка SX1276 в FSK TX режиме без остального железа КП.
 * Передаёт строку FOX_MORSE в цикле без перерыва на частоте FOX_FREQ_HZ.
 * FM-приёмник (например Baofeng) должен слышать тон 800 Гц в ритме морзянки.
 *
 * Пины (совпадают с основным проектом):
 *   SCK  = GPIO15, MOSI = GPIO2, MISO = GPIO4
 *   LoRa CS   = GPIO13
 *   LoRa DIO0 = GPIO12  (TxDone IRQ, не используется в FSK, но нужен для init)
 *   LoRa DIO2 = GPIO16  (FSK прямая модуляция: меандр → FM тон 800 Гц)
 *   SPI RST   = GPIO14
 *
 * Параметры (менять здесь):
 */
#define FOX_FREQ_HZ   433920000UL   // Несущая частота, Гц
#define FOX_FDEV_HZ        3500UL   // Девиация FSK, Гц (±3.5 кГц → NFM)
#define FOX_TONE_HZ         800UL   // Частота аудио тона морзянки, Гц
#define FOX_MORSE           "PANKI HOY"   // Строка морзянки (макс 15 символов)

// ——————————————————————————————————————————————————————————————————————————

#include "lora/sx1276.h"
#include "lora/fox_mode.h"
#include "config/types.h"

#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <cstring>
#include <cinttypes>

static const char *TAG = "LoRaFSKTest";

// Пины SPI шины
static constexpr gpio_num_t GPIO_SPI_SCK  = GPIO_NUM_15;
static constexpr gpio_num_t GPIO_SPI_MOSI = GPIO_NUM_2;
static constexpr gpio_num_t GPIO_SPI_MISO = GPIO_NUM_4;
static constexpr gpio_num_t GPIO_SPI_RST  = GPIO_NUM_14;
static constexpr gpio_num_t GPIO_LORA_CS  = GPIO_NUM_13;
static constexpr gpio_num_t GPIO_LORA_DIO0 = GPIO_NUM_12;
static constexpr gpio_num_t GPIO_LORA_DIO2 = GPIO_NUM_16;

extern "C" void app_main()
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  FoxAss LoRa FSK Test                 ");
    ESP_LOGI(TAG, "  Freq: %lu Hz  Fdev: %lu Hz            ",
             (unsigned long)FOX_FREQ_HZ, (unsigned long)FOX_FDEV_HZ);
    ESP_LOGI(TAG, "  Morse: \"%s\"                          ", FOX_MORSE);
    ESP_LOGI(TAG, "========================================");

    // ——— Инициализация SPI шины ————————————————————————————————————————————
    spi_bus_config_t bus_cfg = {};
    bus_cfg.mosi_io_num   = GPIO_SPI_MOSI;
    bus_cfg.miso_io_num   = GPIO_SPI_MISO;
    bus_cfg.sclk_io_num   = GPIO_SPI_SCK;
    bus_cfg.quadwp_io_num = -1;
    bus_cfg.quadhd_io_num = -1;
    bus_cfg.max_transfer_sz = 256;

    esp_err_t ret = spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI bus init failed: %s", esp_err_to_name(ret));
        return;
    }
    ESP_LOGI(TAG, "SPI bus initialized");

    // ——— Инициализация SX1276 ——————————————————————————————————————————————
    static lora::SX1276 radio;

    radio.setDio2Gpio(GPIO_LORA_DIO2);

    ret = radio.init(SPI2_HOST, GPIO_LORA_CS, GPIO_LORA_DIO0, GPIO_SPI_RST);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SX1276 init failed: %s", esp_err_to_name(ret));
        return;
    }
    ESP_LOGI(TAG, "SX1276 initialized");

    // ——— Конфигурация Fox ——————————————————————————————————————————————————
    // Используем только fox-поля; остальные поля не нужны для FoxMode.
    static config::StationConfig cfg;
    cfg.fox_freq_hz  = FOX_FREQ_HZ;
    cfg.fox_fdev_hz  = FOX_FDEV_HZ;
    cfg.fox_tone_hz  = FOX_TONE_HZ;
    cfg.lora_freq_hz = FOX_FREQ_HZ; // не используется (fox_off_sec=0 → нет возврата в LoRa)
    cfg.fox_on_sec   = 0xFFFFFFFFUL; // передавать бесконечно
    cfg.fox_off_sec  = 0;             // без паузы
    static_assert(sizeof(FOX_MORSE) - 1 <= 15, "FOX_MORSE must be <= 15 chars");
    memcpy(cfg.fox_morse, FOX_MORSE, sizeof(FOX_MORSE));

    // ——— Запуск Fox режима ————————————————————————————————————————————————
    static lora::FoxMode fox(radio, GPIO_LORA_DIO2, cfg);
    fox.start();

    ESP_LOGI(TAG, "Fox task started — transmitting \"%s\" on %lu Hz",
             FOX_MORSE, (unsigned long)FOX_FREQ_HZ);
    ESP_LOGI(TAG, "DIO2 = GPIO%d  (меандр 800 Гц во время тонов морзянки)", GPIO_LORA_DIO2);
    ESP_LOGI(TAG, "Настройте FM-приёмник на %.3f МГц (NFM), должен слышать морзянку.",
             FOX_FREQ_HZ / 1e6);

    // ——— Мониторинг: лог раз в минуту ————————————————————————————————————
    uint32_t tick = 0;
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(60000));
        tick++;
        ESP_LOGI(TAG, "[%lu мин] Fox running, uptime=%" PRId64 " s",
                 (unsigned long)tick, esp_timer_get_time() / 1000000LL);
    }
}
