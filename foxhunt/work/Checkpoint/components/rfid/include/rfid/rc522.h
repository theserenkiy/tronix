#pragma once

#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include <cstdint>
#include <cstddef>

namespace rfid {

// ——— Регистры RC522 ——————————————————————————————————————————————————————
enum PcdReg : uint8_t {
    CommandReg     = 0x01, TxControlReg  = 0x14, TxASKReg      = 0x15,
    ComIEnReg      = 0x02, ModWidthReg   = 0x24, RFCfgReg      = 0x26,
    ComIrqReg      = 0x04, CRCResultRegH = 0x21, CRCResultRegL = 0x22,
    DivIrqReg      = 0x05, FIFODataReg   = 0x09, FIFOLevelReg  = 0x0A,
    ErrorReg       = 0x06, WaterLevelReg = 0x0B, ControlReg    = 0x0C,
    Status2Reg     = 0x08, BitFramingReg = 0x0D, CollReg       = 0x0E,
    ModeReg        = 0x11, TModeReg      = 0x2A, TPrescalerReg = 0x2B,
    TReloadRegH    = 0x2C, TReloadRegL   = 0x2D, VersionReg    = 0x37,
};

// ——— Команды PCD ——————————————————————————————————————————————————————————
enum PcdCmd : uint8_t {
    PCD_Idle        = 0x00,
    PCD_CalcCRC     = 0x03,
    PCD_Transceive  = 0x0C,
    PCD_MFAuthent   = 0x0E,
    PCD_SoftReset   = 0x0F,
};

// ——— Статусы операций ——————————————————————————————————————————————————
enum class Status : uint8_t {
    OK,
    ERROR,
    COLLISION,
    TIMEOUT,
    NO_ROOM,
    CRC_WRONG,
    MIFARE_NACK,
};

/**
 * Драйвер RC522 через ESP-IDF SPI master.
 * Один SPI bus, уже инициализированный снаружи.
 */
class RC522 {
public:
    RC522() = default;

    /**
     * Инициализировать устройство.
     * @param spi_host  SPI хост (уже инициализированный)
     * @param cs_gpio   GPIO chip select
     * @param rst_gpio  GPIO reset (-1 = не используется)
     */
    esp_err_t init(spi_host_device_t spi_host,
                   gpio_num_t cs_gpio,
                   gpio_num_t rst_gpio = GPIO_NUM_NC);

    /** Программный сброс. */
    void reset();

    /** Прочитать регистр. */
    uint8_t readReg(PcdReg reg);

    /** Записать регистр. */
    void writeReg(PcdReg reg, uint8_t value);

    /** Установить биты в регистре. */
    void setBits(PcdReg reg, uint8_t mask);

    /** Сбросить биты в регистре. */
    void clearBits(PcdReg reg, uint8_t mask);

    /**
     * Передать/принять данные через FIFO (команда PCD_Transceive или PCD_MFAuthent).
     * @param cmd          Команда PCD
     * @param send_data    Данные для отправки
     * @param send_len     Длина отправляемых данных
     * @param recv_data    Буфер приёма (может быть nullptr)
     * @param recv_len     [in/out] размер буфера / принятых байт
     * @param valid_bits   [in/out] биты в последнем байте (7 = полный байт)
     * @param rx_align     Выравнивание приёма
     * @param check_crc    Проверять CRC ответа
     */
    Status communicate(PcdCmd cmd,
                       const uint8_t *send_data, uint8_t send_len,
                       uint8_t *recv_data, uint8_t *recv_len,
                       uint8_t *valid_bits = nullptr,
                       uint8_t rx_align = 0,
                       bool check_crc = false);

    /** Вычислить CRC через аппаратный блок RC522. */
    Status calcCRC(const uint8_t *data, uint8_t len, uint8_t result[2]);

    /** Проверить версию чипа (ожидается 0x91 или 0x92). */
    uint8_t getVersion() { return readReg(VersionReg); }

    spi_device_handle_t spiDev() const { return spi_dev_; }

private:
    void readFifo(uint8_t *data, uint8_t len);
    void writeFifo(const uint8_t *data, uint8_t len);
    void flushFifo();

    spi_device_handle_t spi_dev_ = nullptr;
    gpio_num_t rst_gpio_         = GPIO_NUM_NC;
};

} // namespace rfid
