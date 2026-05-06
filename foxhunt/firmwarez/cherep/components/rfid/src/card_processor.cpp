#include "rfid/card_processor.h"
#include "crypto/hmac.h"
#include "esp_log.h"
#include <cstring>
#include <cinttypes>

static const char *TAG = "CardProc";

namespace rfid {

CardProcessor::CardProcessor(MifareClassic &mifare,
                             const uint8_t  station_key[16],
                             const uint8_t  sector_key[6])
    : mifare_(mifare)
{
    memcpy(station_key_, station_key, 16);
    memcpy(sector_key_,  sector_key,  6);
}

// ——— Упаковка блока КП ——————————————————————————————————————————————————

void CardProcessor::packCpBlock(uint8_t       *block16,
                                uint8_t        cp_num,
                                uint64_t       ts_arrive,
                                uint8_t        flags,
                                uint16_t       reserve,
                                const uint8_t  station_key[16],
                                const Uid     &uid)
{
    memset(block16, 0, 16);

    block16[0] = cp_num;

    // ts_arrive: 8 байт Big Endian, смещение 1
    for (int i = 0; i < 8; i++) {
        block16[1 + i] = static_cast<uint8_t>(ts_arrive >> (56 - i * 8));
    }

    block16[9]  = flags;

    // reserve: 2 байта BE, смещение 10
    block16[10] = static_cast<uint8_t>(reserve >> 8);
    block16[11] = static_cast<uint8_t>(reserve & 0xFF);

    // MAC: 4 байта, смещение 12
    uint8_t mac[4];
    crypto::compute_cp_mac(station_key, cp_num, ts_arrive, flags, reserve,
                           uid.bytes, uid.size, mac);
    memcpy(block16 + 12, mac, 4);
}

// ——— Reselect с проверкой UID ———————————————————————————————————————————

esp_err_t CardProcessor::reselectAndVerify(const Uid &expected_uid)
{
    Uid new_uid;
    Status s = mifare_.reselect(new_uid);
    if (s != Status::OK) {
        ESP_LOGE(TAG, "Reselect failed: %d", (int)s);
        return ESP_ERR_INVALID_STATE;
    }

    if (new_uid.size != expected_uid.size ||
        memcmp(new_uid.bytes, expected_uid.bytes, expected_uid.size) != 0) {
        ESP_LOGE(TAG, "Card changed during operation!");
        return ESP_ERR_INVALID_STATE;
    }

    return ESP_OK;
}

// ——— Аутентификация с ленивой инициализацией ————————————————————————————

esp_err_t CardProcessor::authSector(const Uid &uid, uint8_t sector)
{
    uint8_t trailer = MifareClassic::trailerBlock(sector);

    // Попытка 1: наш ключ
    Status s = mifare_.authenticate(KeyType::KEY_A, trailer, sector_key_, uid);
    if (s == Status::OK) return ESP_OK;

    // Auth провалился → карта ушла в HALT (по спеке MIFARE), crypto сброшена
    // Пересоединяемся
    if (reselectAndVerify(uid) != ESP_OK) return ESP_ERR_INVALID_STATE;

    // Попытка 2: дефолтный ключ (сектор ещё не инициализирован)
    static const uint8_t default_key[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
    s = mifare_.authenticate(KeyType::KEY_A, trailer, default_key, uid);
    if (s != Status::OK) {
        ESP_LOGE(TAG, "Sector %d: neither custom nor default key works", sector);
        return ESP_ERR_INVALID_STATE;
    }

    // Дефолтный ключ сработал → инициализируем сектор (записываем trailer)
    uint8_t trailer_data[16];
    memcpy(trailer_data,      sector_key_, 6);    // Key A
    trailer_data[6]  = 0xFF;                      // Access bits
    trailer_data[7]  = 0x07;
    trailer_data[8]  = 0x80;
    trailer_data[9]  = 0x69;                      // GPB
    memcpy(trailer_data + 10, sector_key_, 6);    // Key B = Key A

    s = mifare_.writeBlock(trailer, trailer_data);
    if (s != Status::OK) {
        ESP_LOGE(TAG, "Sector %d trailer write failed: %d", sector, (int)s);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Sector %d initialized with custom key", sector);

    // Crypto1 сессия всё ещё активна (со старым ключом), можно работать с блоками
    return ESP_OK;
}

// ——— Поиск свободного слота ——————————————————————————————————————————————

esp_err_t CardProcessor::findFreeSlot(const Uid &uid, uint8_t cp_num,
                                      SlotInfo &slot_out, bool &is_duplicate)
{
    slot_out = {};
    is_duplicate = false;
    uint8_t block_data[16];

    for (uint8_t sector = 1; sector <= 14; sector++) {
        // Reselect перед каждым сектором — чистое состояние
        if (reselectAndVerify(uid) != ESP_OK) return ESP_ERR_INVALID_STATE;

        // Auth для этого сектора (с ленивой инициализацией)
        esp_err_t ret = authSector(uid, sector);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Sector %d auth failed, skipping", sector);
            continue;
        }

        for (uint8_t b = 0; b < 3; b++) {
            uint8_t abs_block = MifareClassic::sectorBlock(sector, b);
            Status s = mifare_.readBlock(abs_block, block_data);
            if (s != Status::OK) {
                ESP_LOGW(TAG, "Read block %d failed", abs_block);
                continue;
            }

            if (block_data[0] == 0xFF) {
                // Свободный слот!
                slot_out.sector     = sector;
                slot_out.block_in_s = b;
                slot_out.abs_block  = abs_block;
                slot_out.found      = true;

                ESP_LOGI(TAG, "Free slot: sector=%d block=%d (abs=%d), dup=%d",
                         sector, b, abs_block, is_duplicate);
                return ESP_OK;
            }

            // Проверка на дубль
            if (block_data[0] == cp_num) {
                is_duplicate = true;
            }
        }
    }

    // Очистить состояние карты перед возвратом
    mifare_.haltA();
    mifare_.stopCrypto();

    ESP_LOGE(TAG, "Card full — no free slots");
    return ESP_ERR_NO_MEM;
}

// ——— Запись с верификацией ———————————————————————————————————————————————

esp_err_t CardProcessor::writeAndVerify(const Uid &uid, uint8_t sector,
                                        uint8_t abs_block,
                                        const uint8_t data[16],
                                        int max_retries)
{
    for (int attempt = 0; attempt <= max_retries; attempt++) {
        if (attempt > 0) {
            ESP_LOGW(TAG, "Write retry %d for block %d", attempt, abs_block);
            if (reselectAndVerify(uid) != ESP_OK) return ESP_ERR_INVALID_STATE;
            if (authSector(uid, sector) != ESP_OK) return ESP_ERR_INVALID_STATE;
        }

        // Запись
        Status s = mifare_.writeBlock(abs_block, data);
        if (s != Status::OK) {
            ESP_LOGW(TAG, "Write block %d failed: %d", abs_block, (int)s);
            continue;
        }

        // Верификация: прочитать обратно и сравнить
        uint8_t readback[16];
        s = mifare_.readBlock(abs_block, readback);
        if (s != Status::OK) {
            ESP_LOGW(TAG, "Read-back block %d failed: %d", abs_block, (int)s);
            continue;
        }

        if (memcmp(data, readback, 16) == 0) {
            return ESP_OK;
        }

        ESP_LOGW(TAG, "Write verify mismatch on block %d", abs_block);
    }

    ESP_LOGE(TAG, "Write failed after %d retries for block %d",
             max_retries + 1, abs_block);
    return ESP_FAIL;
}

// ——— Общая логика записи слота ——————————————————————————————————————————

esp_err_t CardProcessor::writeSlot(const Uid &uid, uint8_t cp_num,
                                   uint64_t timestamp, uint8_t flags,
                                   uint16_t reserve, SlotInfo &slot_out)
{
    // Найти свободный слот (с проверкой дублей)
    bool is_duplicate = false;
    esp_err_t ret = findFreeSlot(uid, cp_num, slot_out, is_duplicate);
    if (ret != ESP_OK) return ret;

    slot_out.was_duplicate = is_duplicate;

    // Дубль — всегда пишем флаг 0x01 (независимо от начального flags)
    if (is_duplicate) {
        flags = 0x01;
    }

    // Reselect + auth для целевого сектора
    ret = reselectAndVerify(uid);
    if (ret != ESP_OK) return ret;

    ret = authSector(uid, slot_out.sector);
    if (ret != ESP_OK) return ret;

    // Упаковать и записать с верификацией
    uint8_t block_data[16];
    packCpBlock(block_data, cp_num, timestamp, flags, reserve,
                station_key_, uid);

    ret = writeAndVerify(uid, slot_out.sector, slot_out.abs_block, block_data);
    if (ret != ESP_OK) return ret;

    ESP_LOGI(TAG, "Slot written: cp=%d sector=%d block=%d flags=0x%02X ts=%" PRIu64,
             cp_num, slot_out.sector, slot_out.abs_block, flags, timestamp);

    // Привести карту в чистое состояние
    mifare_.haltA();
    mifare_.stopCrypto();

    return ESP_OK;
}

// ——— writeCpMark (однократная запись) ———————————————————————————————————

esp_err_t CardProcessor::writeCpMark(const Uid &uid,
                                     uint8_t    cp_num,
                                     uint64_t   timestamp,
                                     SlotInfo  &slot_out)
{
    return writeSlot(uid, cp_num, timestamp, 0x00, 0x0000, slot_out);
}

// ——— writeFirstTouch ————————————————————————————————————————————————————

esp_err_t CardProcessor::writeFirstTouch(const Uid &uid,
                                         uint8_t    cp_num,
                                         uint64_t   timestamp,
                                         SlotInfo  &slot_out)
{
    return writeSlot(uid, cp_num, timestamp, 0x03, 0x0000, slot_out);
}

// ——— writeSecondTouch ————————————————————————————————————————————————————

esp_err_t CardProcessor::writeSecondTouch(const Uid   &uid,
                                          const SlotInfo &slot,
                                          uint8_t      cp_num,
                                          uint64_t     ts_arrive,
                                          uint16_t     reserve_val)
{
    if (!slot.found) return ESP_ERR_INVALID_ARG;

    // Reselect + auth
    esp_err_t ret = reselectAndVerify(uid);
    if (ret != ESP_OK) return ret;

    ret = authSector(uid, slot.sector);
    if (ret != ESP_OK) return ret;

    // flags=0x00 (завершено), reserve=результат испытания
    uint8_t block_data[16];
    packCpBlock(block_data, cp_num, ts_arrive, 0x00, reserve_val,
                station_key_, uid);

    ret = writeAndVerify(uid, slot.sector, slot.abs_block, block_data);
    if (ret != ESP_OK) return ret;

    ESP_LOGI(TAG, "Second touch: cp=%d reserve=0x%04X", cp_num, reserve_val);

    mifare_.haltA();
    mifare_.stopCrypto();

    return ESP_OK;
}

// ——— readParticipant —————————————————————————————————————————————————————

esp_err_t CardProcessor::readParticipant(const Uid &uid, ParticipantInfo &info)
{
    // Reselect для чистого состояния
    esp_err_t ret = reselectAndVerify(uid);
    if (ret != ESP_OK) return ret;

    ret = authSector(uid, 0);
    if (ret != ESP_OK) return ret;

    uint8_t block1[16], block2[16];

    if (mifare_.readBlock(1, block1) != Status::OK) {
        mifare_.haltA();
        mifare_.stopCrypto();
        return ESP_FAIL;
    }
    if (mifare_.readBlock(2, block2) != Status::OK) {
        mifare_.haltA();
        mifare_.stopCrypto();
        return ESP_FAIL;
    }

    mifare_.haltA();
    mifare_.stopCrypto();

    // Разбор блока 1
    info.event_id       = (block1[0] << 8) | block1[1];
    info.participant_id = (block1[2] << 8) | block1[3];
    info.group_id       = block1[4];
    info.format         = block1[5];
    info.flags          = block1[6];

    // Блок 2 — имя участника (16 байт UTF-8)
    memcpy(info.name, block2, 16);
    info.name[16] = '\0';

    return ESP_OK;
}

// ——— initSector (публичный, для ручного вызова) —————————————————————————

esp_err_t CardProcessor::initSector(const Uid &uid, uint8_t sector)
{
    esp_err_t ret = reselectAndVerify(uid);
    if (ret != ESP_OK) return ret;

    ret = authSector(uid, sector);

    mifare_.haltA();
    mifare_.stopCrypto();

    return ret;
}

// ——— clearSlots — очистка всех слотов (заполнение 0xFF) —————————————————

esp_err_t CardProcessor::clearSlots(const Uid &uid)
{
    static const uint8_t empty_block[16] = {
        0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF,
        0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF
    };

    int ok = 0, fail = 0;

    for (uint8_t sector = 1; sector <= 14; sector++) {
        if (reselectAndVerify(uid) != ESP_OK) {
            fail += 3;
            continue;
        }

        esp_err_t ret = authSector(uid, sector);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "clearSlots: sector %d auth failed", sector);
            fail += 3;
            continue;
        }

        for (uint8_t b = 0; b < 3; b++) {
            uint8_t abs_block = MifareClassic::sectorBlock(sector, b);
            Status s = mifare_.writeBlock(abs_block, empty_block);
            if (s == Status::OK) {
                ok++;
            } else {
                ESP_LOGW(TAG, "clearSlots: block %d write failed", abs_block);
                fail++;
            }
        }
    }

    mifare_.haltA();
    mifare_.stopCrypto();

    ESP_LOGI(TAG, "clearSlots: ok=%d fail=%d", ok, fail);
    return (fail == 0) ? ESP_OK : ESP_FAIL;
}

} // namespace rfid
