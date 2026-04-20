#pragma once

#include "rfid/rc522.h"
#include <cstdint>

namespace rfid {

// ——— PICC команды ————————————————————————————————————————————————————————
enum PiccCmd : uint8_t {
    PICC_REQA        = 0x26,
    PICC_WUPA        = 0x52,
    PICC_CT          = 0x88, // Cascade Tag
    PICC_SEL_CL1     = 0x93,
    PICC_SEL_CL2     = 0x95,
    PICC_SEL_CL3     = 0x97,
    PICC_HLTA        = 0x50,
    PICC_MF_AUTH_A   = 0x60,
    PICC_MF_AUTH_B   = 0x61,
    PICC_MF_READ     = 0x30,
    PICC_MF_WRITE    = 0xA0,
};

// Тип ключа для аутентификации
enum class KeyType : uint8_t {
    KEY_A = PICC_MF_AUTH_A,
    KEY_B = PICC_MF_AUTH_B,
};

// UID карты (до 10 байт)
struct Uid {
    uint8_t bytes[10] = {};
    uint8_t size      = 0;
    uint8_t sak       = 0; // Select Acknowledge
};

/**
 * MIFARE Classic операции поверх RC522.
 * Не потокобезопасно — синхронизация на уровне задач.
 */
class MifareClassic {
public:
    explicit MifareClassic(RC522 &pcd) : pcd_(pcd) {}

    /**
     * Проверить наличие новой карты в поле.
     * Внутри: REQA → если ATQA получен → true.
     */
    bool isCardPresent();

    /**
     * Прочитать UID карты (REQA + anticollision + select).
     */
    Status readCardSerial(Uid &uid);

    /**
     * Аутентифицироваться с блоком, используя ключ.
     * @param key_type  KEY_A или KEY_B
     * @param block     Номер блока (0..63)
     * @param key       6-байтный ключ
     * @param uid       UID карты (нужен для crypto1)
     */
    Status authenticate(KeyType key_type, uint8_t block,
                        const uint8_t key[6], const Uid &uid);

    /**
     * Прочитать блок (16 байт). Карта должна быть аутентифицирована.
     */
    Status readBlock(uint8_t block, uint8_t data[16]);

    /**
     * Записать блок (16 байт). Карта должна быть аутентифицирована.
     */
    Status writeBlock(uint8_t block, const uint8_t data[16]);

    /** Остановить карту (HLTA). */
    void haltA();

    /** Остановить crypto1 на RC522. */
    void stopCrypto();

    /**
     * Пересоединение с картой: haltA → stopCrypto → WUPA+anticoll+SELECT.
     * Если карта застряла в ACTIVE* — fallback через перезапуск RF-поля.
     * @param uid  [out] UID карты после пересоединения
     * @return Status::OK при успехе
     */
    Status reselect(Uid &uid);

    /** Сконвертировать сектор+блок_внутри_сектора в абсолютный номер блока. */
    static uint8_t sectorBlock(uint8_t sector, uint8_t block_in_sector) {
        return sector * 4 + block_in_sector;
    }

    /** Блок sector trailer для данного сектора. */
    static uint8_t trailerBlock(uint8_t sector) {
        return sector * 4 + 3;
    }

private:
    Status requestA(uint8_t *atqa);
    Status anticollSelect(Uid &uid);

    RC522 &pcd_;
};

} // namespace rfid
