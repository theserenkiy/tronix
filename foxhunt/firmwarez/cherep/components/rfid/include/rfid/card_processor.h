#pragma once

#include "rfid/mifare.h"
#include "esp_err.h"
#include <cstdint>

namespace rfid {

// Информация об участнике из сектора 0
struct ParticipantInfo {
    uint16_t event_id      = 0;
    uint16_t participant_id= 0;
    uint8_t  group_id      = 0;
    uint8_t  format        = 0;
    uint8_t  flags         = 0;
    char     name[17]      = {}; // 16 байт + '\0'
};

// Результат поиска слота
struct SlotInfo {
    uint8_t sector       = 0;
    uint8_t block_in_s   = 0; // 0..2
    uint8_t abs_block    = 0; // абсолютный номер блока
    bool    found        = false;
    bool    was_duplicate = false; // cp_num уже был на карте
};

/**
 * Бизнес-логика работы с картой MIFARE Classic 1K.
 *
 * Управляет состоянием карты (reselect между секторами),
 * верифицирует записи (read-back), обнаруживает дубли.
 */
class CardProcessor {
public:
    CardProcessor(MifareClassic &mifare,
                  const uint8_t  station_key[16],
                  const uint8_t  sector_key[6]);

    /**
     * Однократная запись КП.
     * Находит свободный слот, записывает cp_num + timestamp.
     * flags=0x00 (норма) или 0x01 (дубль, если cp_num уже есть на карте).
     * reserve=0x0000.
     */
    esp_err_t writeCpMark(const Uid &uid,
                          uint8_t    cp_num,
                          uint64_t   timestamp,
                          SlotInfo  &slot_out);

    /**
     * Первое касание двухкасательного КП.
     * flags=0x03 (ожидание), reserve=0x0000.
     */
    esp_err_t writeFirstTouch(const Uid &uid,
                              uint8_t    cp_num,
                              uint64_t   timestamp,
                              SlotInfo  &slot_out);

    /**
     * Второе касание (после испытания).
     * Перезаписывает тот же слот: flags=0x00, reserve=result_value.
     */
    esp_err_t writeSecondTouch(const Uid &uid,
                               const SlotInfo &slot,
                               uint8_t      cp_num,
                               uint64_t     ts_arrive,
                               uint16_t     reserve_val);

    /**
     * Прочитать данные участника из сектора 0.
     */
    esp_err_t readParticipant(const Uid &uid, ParticipantInfo &info);

    /**
     * Инициализировать сектор (записать trailer с нашими ключами).
     * Обычно вызывается автоматически из authSector().
     */
    esp_err_t initSector(const Uid &uid, uint8_t sector);

    /**
     * Очистить все слоты (секторы 1-14): записать 0xFF во все блоки данных.
     * Вызывать при регистрации карты перед использованием на трассе.
     */
    esp_err_t clearSlots(const Uid &uid);

private:
    // Reselect + проверка, что карта та же
    esp_err_t reselectAndVerify(const Uid &expected_uid);

    // Auth с нашим ключом; если сектор не инициализирован — инициализирует
    esp_err_t authSector(const Uid &uid, uint8_t sector);

    // Поиск свободного слота + проверка дублей
    esp_err_t findFreeSlot(const Uid &uid, uint8_t cp_num,
                           SlotInfo &slot_out, bool &is_duplicate);

    // Запись с верификацией (write → read-back → compare, до max_retries)
    esp_err_t writeAndVerify(const Uid &uid, uint8_t sector,
                             uint8_t abs_block, const uint8_t data[16],
                             int max_retries = 2);

    // Запись отметки (общая логика для writeCpMark / writeFirstTouch)
    esp_err_t writeSlot(const Uid &uid, uint8_t cp_num, uint64_t timestamp,
                        uint8_t flags, uint16_t reserve, SlotInfo &slot_out);

    void packCpBlock(uint8_t       *block16,
                     uint8_t        cp_num,
                     uint64_t       ts_arrive,
                     uint8_t        flags,
                     uint16_t       reserve,
                     const uint8_t  station_key[16],
                     const Uid     &uid);

    MifareClassic &mifare_;
    uint8_t        station_key_[16];
    uint8_t        sector_key_[6];
};

} // namespace rfid
