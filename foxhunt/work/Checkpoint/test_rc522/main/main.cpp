/**
 * FoxAss RC522 Test — безопасный полный цикл.
 *
 * Ключ сектора = FFFFFFFFFFFF (дефолтный) — карта всегда читаема телефоном.
 * Тест: UID → writeCpMark(cp=1) → writeCpMark(cp=2) → writeCpMark(cp=1, дубль) → дамп.
 *
 * Пины:
 *   SCK=G15, MOSI=G2, MISO=G4, RST=G14
 *   CS RFID=G5, CS LoRa=G13 (HIGH)
 */

#include "rfid/rc522.h"
#include "rfid/mifare.h"
#include "rfid/card_processor.h"

#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cstring>

static const char *TAG = "RC522Test";

static constexpr gpio_num_t GPIO_SPI_SCK   = GPIO_NUM_15;
static constexpr gpio_num_t GPIO_SPI_MOSI  = GPIO_NUM_2;
static constexpr gpio_num_t GPIO_SPI_MISO  = GPIO_NUM_4;
static constexpr gpio_num_t GPIO_SPI_RST   = GPIO_NUM_14;
static constexpr gpio_num_t GPIO_RFID_CS   = GPIO_NUM_5;
static constexpr gpio_num_t GPIO_LORA_CS   = GPIO_NUM_13;

// Ключи: дефолтные (безопасно — карта всегда читаема)
static const uint8_t SECTOR_KEY[6]   = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
static const uint8_t STATION_KEY[16] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,
                                        0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10};

// ——— Дамп блока ——————————————————————————————————————————————————————————
static void dumpBlock(rfid::MifareClassic &mifare, const rfid::Uid &uid,
                      uint8_t sector, uint8_t block_in_sector,
                      const uint8_t key[6])
{
    uint8_t trailer = rfid::MifareClassic::trailerBlock(sector);
    rfid::Status s = mifare.authenticate(rfid::KeyType::KEY_A, trailer, key, uid);
    if (s != rfid::Status::OK) {
        ESP_LOGW(TAG, "  Auth sector %d failed: %d", sector, (int)s);
        mifare.stopCrypto();
        return;
    }

    uint8_t abs_block = rfid::MifareClassic::sectorBlock(sector, block_in_sector);
    uint8_t data[16];
    s = mifare.readBlock(abs_block, data);
    mifare.stopCrypto();

    if (s != rfid::Status::OK) {
        ESP_LOGW(TAG, "  Read block %d failed: %d", abs_block, (int)s);
        return;
    }

    ESP_LOGI(TAG, "  [S%d B%d abs=%d] %02X %02X %02X %02X %02X %02X %02X %02X "
                  "%02X %02X %02X %02X %02X %02X %02X %02X",
             sector, block_in_sector, abs_block,
             data[0],  data[1],  data[2],  data[3],
             data[4],  data[5],  data[6],  data[7],
             data[8],  data[9],  data[10], data[11],
             data[12], data[13], data[14], data[15]);

    // Расшифровка если не пустой слот
    if (data[0] != 0xFF) {
        uint8_t flags = data[9];
        uint16_t reserve = (data[10] << 8) | data[11];
        const char *flag_str = "???";
        switch (flags) {
            case 0x00: flag_str = "NORM"; break;
            case 0x01: flag_str = "DUP";  break;
            case 0x02: flag_str = "ERR";  break;
            case 0x03: flag_str = "WAIT"; break;
        }
        ESP_LOGI(TAG, "    cp=%d flags=0x%02X(%s) reserve=0x%04X",
                 data[0], flags, flag_str, reserve);
    }
}

// ——— Дамп всех записанных слотов ————————————————————————————————————————
static void dumpAllSlots(rfid::MifareClassic &mifare, const rfid::Uid &uid,
                         const uint8_t key[6])
{
    ESP_LOGI(TAG, "--- Дамп всех слотов ---");
    for (uint8_t sector = 1; sector <= 14; sector++) {
        // Reselect перед каждым сектором
        rfid::Uid tmp_uid;
        mifare.reselect(tmp_uid);

        uint8_t trailer = rfid::MifareClassic::trailerBlock(sector);
        rfid::Status s = mifare.authenticate(rfid::KeyType::KEY_A, trailer, key, uid);
        if (s != rfid::Status::OK) {
            // Сектор недоступен — пропускаем
            continue;
        }

        bool sector_empty = true;
        for (uint8_t b = 0; b < 3; b++) {
            uint8_t abs_block = rfid::MifareClassic::sectorBlock(sector, b);
            uint8_t data[16];
            if (mifare.readBlock(abs_block, data) != rfid::Status::OK) continue;
            if (data[0] == 0xFF) {
                // Свободный слот — все последующие тоже свободны
                if (!sector_empty) {
                    ESP_LOGI(TAG, "  [S%d B%d] (свободен)", sector, b);
                }
                mifare.haltA();
                mifare.stopCrypto();
                if (sector_empty) goto next_sector;
                goto done;
            }
            sector_empty = false;

            uint8_t flags = data[9];
            uint16_t reserve = (data[10] << 8) | data[11];
            const char *flag_str = "???";
            switch (flags) {
                case 0x00: flag_str = "NORM"; break;
                case 0x01: flag_str = "DUP";  break;
                case 0x02: flag_str = "ERR";  break;
                case 0x03: flag_str = "WAIT"; break;
            }
            ESP_LOGI(TAG, "  [S%d B%d abs=%d] cp=%d flags=%s reserve=0x%04X",
                     sector, b, abs_block, data[0], flag_str, reserve);
        }

        mifare.haltA();
        mifare.stopCrypto();
        next_sector:;
    }
    done:

    // Финальный reselect + halt
    rfid::Uid tmp;
    if (mifare.reselect(tmp) == rfid::Status::OK) {
        mifare.haltA();
        mifare.stopCrypto();
    }

    ESP_LOGI(TAG, "--- Конец дампа ---");
}

// ——— Запуск всех тестов ————————————————————————————————————————————————
static void runTests(rfid::CardProcessor &proc, rfid::MifareClassic &mifare,
                     const rfid::Uid &uid, const uint8_t *key)
{
    esp_err_t ret;
    rfid::SlotInfo slot;

    // Шаг 0: очистка всех слотов
    ESP_LOGI(TAG, "--- Шаг 0: clearSlots (подготовка карты) ---");
    ret = proc.clearSlots(uid);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "clearSlots FAIL: %s", esp_err_to_name(ret));
        return;
    }
    ESP_LOGI(TAG, "clearSlots OK");

    // Тест 1: writeCpMark(cp=1)
    ESP_LOGI(TAG, "--- Тест 1: writeCpMark(cp=1) ---");
    ret = proc.writeCpMark(uid, 1, 1700000001ULL, slot);
    if (ret != ESP_OK) { ESP_LOGE(TAG, "FAIL: %s", esp_err_to_name(ret)); return; }
    ESP_LOGI(TAG, "OK: sector=%d block=%d", slot.sector, slot.abs_block);

    // Тест 2: writeCpMark(cp=2)
    ESP_LOGI(TAG, "--- Тест 2: writeCpMark(cp=2) ---");
    ret = proc.writeCpMark(uid, 2, 1700000002ULL, slot);
    if (ret != ESP_OK) { ESP_LOGE(TAG, "FAIL: %s", esp_err_to_name(ret)); return; }
    ESP_LOGI(TAG, "OK: sector=%d block=%d", slot.sector, slot.abs_block);

    // Тест 3: writeCpMark(cp=1) повторно — должен быть дубль (flags=0x01)
    ESP_LOGI(TAG, "--- Тест 3: writeCpMark(cp=1) повторно (ожидаем DUP) ---");
    ret = proc.writeCpMark(uid, 1, 1700000003ULL, slot);
    if (ret != ESP_OK) { ESP_LOGE(TAG, "FAIL: %s", esp_err_to_name(ret)); return; }
    ESP_LOGI(TAG, "OK: sector=%d block=%d", slot.sector, slot.abs_block);

    // Тест 4: двухкасательный КП (cp=3)
    ESP_LOGI(TAG, "--- Тест 4: двухкасательный КП (cp=3) ---");
    ret = proc.writeFirstTouch(uid, 3, 1700000004ULL, slot);
    if (ret != ESP_OK) { ESP_LOGE(TAG, "First touch FAIL: %s", esp_err_to_name(ret)); return; }
    ESP_LOGI(TAG, "First touch OK: sector=%d block=%d", slot.sector, slot.abs_block);

    ret = proc.writeSecondTouch(uid, slot, 3, 1700000004ULL, 0x1234);
    if (ret != ESP_OK) { ESP_LOGE(TAG, "Second touch FAIL: %s", esp_err_to_name(ret)); return; }
    ESP_LOGI(TAG, "Second touch OK: reserve=0x1234");

    // Дамп всех записанных слотов
    ESP_LOGI(TAG, "");
    dumpAllSlots(mifare, uid, key);

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "=== Все тесты завершены ===");
}

extern "C" void app_main()
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  FoxAss RC522 Safe Test");
    ESP_LOGI(TAG, "  Key = FFFFFFFFFFFF (default)");
    ESP_LOGI(TAG, "========================================");

    // LoRa CS → HIGH
    gpio_config_t lora_cs_cfg = {};
    lora_cs_cfg.pin_bit_mask = (1ULL << GPIO_LORA_CS);
    lora_cs_cfg.mode         = GPIO_MODE_OUTPUT;
    lora_cs_cfg.intr_type    = GPIO_INTR_DISABLE;
    gpio_config(&lora_cs_cfg);
    gpio_set_level(GPIO_LORA_CS, 1);

    // SPI bus
    spi_bus_config_t bus_cfg = {};
    bus_cfg.mosi_io_num   = GPIO_SPI_MOSI;
    bus_cfg.miso_io_num   = GPIO_SPI_MISO;
    bus_cfg.sclk_io_num   = GPIO_SPI_SCK;
    bus_cfg.quadwp_io_num = -1;
    bus_cfg.quadhd_io_num = -1;
    bus_cfg.max_transfer_sz = 256;
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CH_AUTO));

    // RC522
    static rfid::RC522 pcd;
    ESP_ERROR_CHECK(pcd.init(SPI2_HOST, GPIO_RFID_CS, GPIO_SPI_RST));

    static rfid::MifareClassic mifare(pcd);
    static rfid::CardProcessor proc(mifare, STATION_KEY, SECTOR_KEY);

    ESP_LOGI(TAG, "RC522 ready. Поднеси карту...");

    // ——— Главный цикл: ждём карту, тестируем, ждём убирания ——————————————
    while (true) {
        rfid::Uid uid;
        if (mifare.readCardSerial(uid) != rfid::Status::OK) {
            vTaskDelay(pdMS_TO_TICKS(500));
            continue;
        }

        // Карта найдена
        mifare.haltA();
        mifare.stopCrypto();

        ESP_LOGI(TAG, "");
        ESP_LOGI(TAG, "=== Карта: %02X %02X %02X %02X  SAK=0x%02X ===",
                 uid.bytes[0], uid.bytes[1], uid.bytes[2], uid.bytes[3], uid.sak);

        // Запустить все тесты
        runTests(proc, mifare, uid, SECTOR_KEY);

        // Гарантируем чистое состояние карты перед ожиданием
        mifare.haltA();
        mifare.stopCrypto();

        // Ждём убирания карты
        while (true) {
            rfid::Uid check;
            // WUPA разбудит HALT карту — если ответила, значит ещё на месте
            if (mifare.readCardSerial(check) != rfid::Status::OK) {
                ESP_LOGI(TAG, "Карта убрана. Жду следующую...");
                break;
            }
            mifare.haltA();
            mifare.stopCrypto();
            vTaskDelay(pdMS_TO_TICKS(500));
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
