#include "rfid/mifare.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cstring>

static const char *TAG = "MIFARE";

namespace rfid {

// ——— Утилиты ——————————————————————————————————————————————————————————————

// Проверка UID BCC (XOR всех байт должен быть 0)
static bool checkBCC(const uint8_t *uid, uint8_t count, uint8_t bcc)
{
    uint8_t xor_val = 0;
    for (uint8_t i = 0; i < count; i++) xor_val ^= uid[i];
    return xor_val == bcc;
}

// ——— isCardPresent ————————————————————————————————————————————————————————

bool MifareClassic::isCardPresent()
{
    pcd_.clearBits(CollReg, 0x80); // ValuesAfterColl=0

    uint8_t atqa[2];
    Status s = requestA(atqa);
    return (s == Status::OK || s == Status::COLLISION);
}

// ——— requestA ————————————————————————————————————————————————————————————

Status MifareClassic::requestA(uint8_t *atqa)
{
    // WUPA (0x52) вместо REQA (0x26): будит карты как в IDLE, так и в HALT state.
    // После haltA() карта переходит в HALT и не отвечает на REQA — только на WUPA.
    pcd_.clearBits(CollReg, 0x80);
    pcd_.writeReg(BitFramingReg, 0x07); // TxLastBits = 7 (7-битный фрейм)

    uint8_t cmd = PICC_WUPA;
    uint8_t len = 2;
    uint8_t vb  = 0;

    Status s = pcd_.communicate(PCD_Transceive, &cmd, 1, atqa, &len, &vb);

    if (s != Status::OK && s != Status::COLLISION) return s;
    if (len != 2 || vb != 0) return Status::ERROR;

    return Status::OK;
}

// ——— readCardSerial (REQA + anticoll + select) ————————————————————————————

Status MifareClassic::readCardSerial(Uid &uid)
{
    uint8_t atqa[2];
    Status s = requestA(atqa);
    if (s != Status::OK) return s;

    return anticollSelect(uid);
}

// ——— anticollSelect ——————————————————————————————————————————————————————

Status MifareClassic::anticollSelect(Uid &uid)
{
    // Только cascade level 1 для 4-байтного UID (MIFARE Classic 1K)
    // Если придёт 7-байтный UID, нужен cascade level 2 — пока не реализуем

    pcd_.clearBits(CollReg, 0x80);

    uint8_t cascade_level = PICC_SEL_CL1;
    uint8_t tx_buf[9];
    uint8_t rx_buf[5];

    // Anti-collision: NVB=20 (2 bytes, 0 bits)
    tx_buf[0] = cascade_level;
    tx_buf[1] = 0x20; // NVB

    pcd_.writeReg(BitFramingReg, 0x00);

    uint8_t rx_len = sizeof(rx_buf);
    uint8_t vb     = 0;
    Status s = pcd_.communicate(PCD_Transceive, tx_buf, 2, rx_buf, &rx_len, &vb);

    if (s != Status::OK) return s;
    if (rx_len != 5) return Status::ERROR;

    // Проверить BCC
    if (!checkBCC(rx_buf, 4, rx_buf[4])) {
        ESP_LOGW(TAG, "BCC mismatch");
        return Status::ERROR;
    }

    // Select: NVB=70 (7 bytes, 0 bits)
    tx_buf[0] = cascade_level;
    tx_buf[1] = 0x70; // NVB = полный UID
    memcpy(tx_buf + 2, rx_buf, 5); // 4 UID bytes + BCC

    // Добавить CRC к select
    uint8_t crc[2];
    s = pcd_.calcCRC(tx_buf, 7, crc);
    if (s != Status::OK) return s;

    tx_buf[7] = crc[0];
    tx_buf[8] = crc[1];

    uint8_t sak_buf[3]; // SAK(1) + CRC(2)
    rx_len = sizeof(sak_buf);
    vb = 0;

    s = pcd_.communicate(PCD_Transceive, tx_buf, 9, sak_buf, &rx_len, &vb,
                         0, true); // check_crc=true
    if (s != Status::OK) return s;
    if (rx_len < 1) return Status::ERROR;

    uid.sak  = sak_buf[0];
    uid.size = 4;
    memcpy(uid.bytes, rx_buf, 4);

    ESP_LOGI(TAG, "Card UID: %02X %02X %02X %02X  SAK=%02X",
             uid.bytes[0], uid.bytes[1], uid.bytes[2], uid.bytes[3], uid.sak);

    return Status::OK;
}

// ——— authenticate ————————————————————————————————————————————————————————

Status MifareClassic::authenticate(KeyType key_type, uint8_t block,
                                   const uint8_t key[6], const Uid &uid)
{
    uint8_t tx[12];
    tx[0] = static_cast<uint8_t>(key_type);
    tx[1] = block;
    memcpy(tx + 2, key, 6);
    memcpy(tx + 8, uid.bytes, 4);

    uint8_t rx_len = 0;
    Status s = pcd_.communicate(PCD_MFAuthent, tx, 12, nullptr, &rx_len);

    if (s != Status::OK) {
        ESP_LOGW(TAG, "Auth failed for block %d: status=%d", block, (int)s);
        return s;
    }

    // Проверить флаг MFCrypto1On
    if (!(pcd_.readReg(Status2Reg) & 0x08)) {
        ESP_LOGE(TAG, "Crypto1 not active after auth");
        return Status::ERROR;
    }

    return Status::OK;
}

// ——— readBlock ————————————————————————————————————————————————————————————

Status MifareClassic::readBlock(uint8_t block, uint8_t data[16])
{
    uint8_t tx[4];
    tx[0] = PICC_MF_READ;
    tx[1] = block;

    uint8_t crc[2];
    Status s = pcd_.calcCRC(tx, 2, crc);
    if (s != Status::OK) return s;
    tx[2] = crc[0];
    tx[3] = crc[1];

    uint8_t rx[18]; // 16 данных + 2 CRC
    uint8_t rx_len = sizeof(rx);

    s = pcd_.communicate(PCD_Transceive, tx, 4, rx, &rx_len, nullptr, 0, true);
    if (s != Status::OK) {
        ESP_LOGW(TAG, "Read block %d failed: %d", block, (int)s);
        return s;
    }

    if (rx_len < 16) return Status::ERROR;
    memcpy(data, rx, 16);
    return Status::OK;
}

// ——— writeBlock ————————————————————————————————————————————————————————————

Status MifareClassic::writeBlock(uint8_t block, const uint8_t data[16])
{
    // Шаг 1: отправить команду записи + номер блока
    uint8_t tx[4];
    tx[0] = PICC_MF_WRITE;
    tx[1] = block;

    uint8_t crc[2];
    Status s = pcd_.calcCRC(tx, 2, crc);
    if (s != Status::OK) return s;
    tx[2] = crc[0];
    tx[3] = crc[1];

    uint8_t ack[1];
    uint8_t ack_len = 1;
    uint8_t vb = 0;

    s = pcd_.communicate(PCD_Transceive, tx, 4, ack, &ack_len, &vb);
    if (s != Status::OK) return s;
    if (ack_len != 1 || (ack[0] & 0x0F) != 0x0A) {
        // 0x0A = ACK от карты
        ESP_LOGW(TAG, "Write cmd ACK failed: 0x%02X", ack_len > 0 ? ack[0] : 0xFF);
        return Status::MIFARE_NACK;
    }

    // Шаг 2: отправить 16 байт данных
    uint8_t tx2[18];
    memcpy(tx2, data, 16);
    s = pcd_.calcCRC(tx2, 16, crc);
    if (s != Status::OK) return s;
    tx2[16] = crc[0];
    tx2[17] = crc[1];

    ack_len = 1;
    vb = 0;
    s = pcd_.communicate(PCD_Transceive, tx2, 18, ack, &ack_len, &vb);
    if (s != Status::OK) return s;
    if (ack_len != 1 || (ack[0] & 0x0F) != 0x0A) {
        ESP_LOGW(TAG, "Write data ACK failed: 0x%02X", ack_len > 0 ? ack[0] : 0xFF);
        return Status::MIFARE_NACK;
    }

    return Status::OK;
}

// ——— haltA / stopCrypto ——————————————————————————————————————————————————

void MifareClassic::haltA()
{
    uint8_t tx[4];
    tx[0] = PICC_HLTA;
    tx[1] = 0x00;

    uint8_t crc[2];
    if (pcd_.calcCRC(tx, 2, crc) == Status::OK) {
        tx[2] = crc[0];
        tx[3] = crc[1];
    }

    uint8_t rx_len = 0;
    pcd_.communicate(PCD_Transceive, tx, 4, nullptr, &rx_len);
}

void MifareClassic::stopCrypto()
{
    pcd_.clearBits(Status2Reg, 0x08); // сбросить MFCrypto1On
}

// ——— reselect ————————————————————————————————————————————————————————————

Status MifareClassic::reselect(Uid &uid)
{
    // Шаг 1: Если crypto активна — отправить зашифрованный HLTA
    // (карта в ACTIVE* получит валидный HLTA и перейдёт в HALT)
    haltA();
    stopCrypto();

    // Шаг 2: WUPA + anticoll + SELECT
    Status s = readCardSerial(uid);
    if (s == Status::OK) return Status::OK;

    // Шаг 3: Fallback — перезапуск RF-поля
    // Карта могла застрять в ACTIVE* (если ранее stopCrypto был вызван
    // без haltA и карта не получила корректный HLTA)
    ESP_LOGW(TAG, "reselect: WUPA failed, power-cycling RF field");
    pcd_.clearBits(TxControlReg, 0x03);    // RF off
    vTaskDelay(pdMS_TO_TICKS(50));
    pcd_.setBits(TxControlReg, 0x03);      // RF on
    vTaskDelay(pdMS_TO_TICKS(10));

    return readCardSerial(uid);
}

} // namespace rfid
