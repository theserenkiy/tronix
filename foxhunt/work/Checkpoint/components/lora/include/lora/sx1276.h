#pragma once

#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_attr.h"
#include "esp_err.h"
#include <cstdint>
#include <cstddef>
#include <functional>

namespace lora {

// ——— Регистры SX1276 ————————————————————————————————————————————————————
namespace Reg {
    static constexpr uint8_t Fifo             = 0x00;
    static constexpr uint8_t OpMode           = 0x01;
    static constexpr uint8_t FrfMsb           = 0x06;
    static constexpr uint8_t FrfMid           = 0x07;
    static constexpr uint8_t FrfLsb           = 0x08;
    static constexpr uint8_t PaConfig         = 0x09;
    static constexpr uint8_t Ocp              = 0x0B;
    static constexpr uint8_t Lna              = 0x0C;
    static constexpr uint8_t FifoAddrPtr      = 0x0D;
    static constexpr uint8_t FifoTxBaseAddr   = 0x0E;
    static constexpr uint8_t FifoRxBaseAddr   = 0x0F;
    static constexpr uint8_t FifoRxCurrentAddr= 0x10;
    static constexpr uint8_t IrqFlagsMask     = 0x11;
    static constexpr uint8_t IrqFlags         = 0x12;
    static constexpr uint8_t RxNbBytes        = 0x13;
    static constexpr uint8_t ModemStat        = 0x18;
    static constexpr uint8_t PktSnrValue      = 0x19;
    static constexpr uint8_t PktRssiValue     = 0x1A;
    static constexpr uint8_t RssiValue        = 0x1B;
    static constexpr uint8_t ModemConfig1     = 0x1D;
    static constexpr uint8_t ModemConfig2     = 0x1E;
    static constexpr uint8_t SymbTimeoutLsb   = 0x1F;
    static constexpr uint8_t PreambleMsb      = 0x20;
    static constexpr uint8_t PreambleLsb      = 0x21;
    static constexpr uint8_t PayloadLength    = 0x22;
    static constexpr uint8_t MaxPayloadLength = 0x23;
    static constexpr uint8_t ModemConfig3     = 0x26;
    static constexpr uint8_t DetectOptimize   = 0x31;
    static constexpr uint8_t InvertIQ        = 0x33;
    static constexpr uint8_t DetectionThres   = 0x37;
    static constexpr uint8_t SyncWord         = 0x39;
    static constexpr uint8_t InvertIQ2       = 0x3B;
    static constexpr uint8_t DioMapping1      = 0x40;
    static constexpr uint8_t DioMapping2      = 0x41;
    static constexpr uint8_t Version          = 0x42;
    static constexpr uint8_t PaDac            = 0x4D;
    // FSK/OOK (LongRangeMode=0)
    static constexpr uint8_t Bitrate_Msb      = 0x02;
    static constexpr uint8_t Bitrate_Lsb      = 0x03;
    static constexpr uint8_t FdevMsb          = 0x04;
    static constexpr uint8_t FdevLsb          = 0x05;
    static constexpr uint8_t OokPeak          = 0x14;
    static constexpr uint8_t OokFix           = 0x15;
    static constexpr uint8_t OokAvg           = 0x16;
    // 0x31 в FSK-режиме = RegPacketConfig2 (в LoRa-режиме = DetectOptimize)
    static constexpr uint8_t PacketConfig2    = 0x31;
}

// IRQ биты (LoRa mode)
static constexpr uint8_t IRQ_RX_DONE   = 0x40;
static constexpr uint8_t IRQ_TX_DONE   = 0x08;
static constexpr uint8_t IRQ_CRC_ERR   = 0x20;
static constexpr uint8_t IRQ_TIMEOUT   = 0x80;

/**
 * Драйвер SX1276.
 * Поддерживает LoRa режим (связь с базой) и OOK режим (Fox).
 */
class SX1276 {
public:
    using RxCallback = std::function<void(const uint8_t *data, uint8_t len, int8_t rssi)>;

    SX1276() = default;

    /**
     * Инициализировать SX1276.
     * @param spi_host  SPI хост
     * @param cs_gpio   CS GPIO
     * @param dio0_gpio DIO0 — IRQ для RxDone/TxDone
     * @param rst_gpio  RST (-1 = не используется)
     */
    esp_err_t init(spi_host_device_t spi_host,
                   gpio_num_t cs_gpio,
                   gpio_num_t dio0_gpio,
                   gpio_num_t rst_gpio = GPIO_NUM_NC);

    /** Настроить LoRa режим.
     * @param freq_hz    Частота, Гц (например 433175000)
     * @param sf         Spreading Factor 6-12 (обычно 7)
     * @param bw_khz     Bandwidth: 125, 250, 500 кГц
     * @param cr         Coding Rate 1-4 (1 = 4/5)
     */
    esp_err_t initLoRa(uint32_t freq_hz, uint8_t sf = 7,
                       uint16_t bw_khz = 125, uint8_t cr = 1);

    /**
     * Настроить FSK режим для Fox (передача морзянки тоном по FM).
     *
     * SX1276 переходит в FSK TX continuous режим. GPIO DIO2 используется
     * как прямой вход модуляции: меандр на DIO2 → девиация частоты →
     * FM-приёмник воспроизводит аудио тон.
     *
     * @param freq_hz   Несущая частота, Гц (напр. 433920000)
     * @param fdev_hz   Девиация, Гц (рекомендуется 3500 для NFM ±5 кГц)
     */
    esp_err_t initFSK(uint32_t freq_hz, uint32_t fdev_hz = 3500);

    /** Передать пакет (LoRa).
     * @param data   Данные
     * @param len    Длина (макс 255)
     * @return ESP_OK при успехе (блокирует до TxDone или таймаута)
     */
    esp_err_t sendPacket(const uint8_t *data, uint8_t len);

    /** Включить приём (LoRa RX continuous). */
    void startReceive();

    /** Установить callback для принятых пакетов. */
    void setRxCallback(RxCallback cb) { rx_cb_ = cb; }

    /** Вызвать из ISR при срабатывании DIO0. */
    void handleDio0Isr();

    /** Получить RSSI последнего пакета (LoRa). */
    int8_t lastRssi() const { return last_rssi_; }

    // Прямое управление несущей (OOK режим)
    void carrierOn()  { gpio_set_level(dio2_gpio_, 1); }
    void carrierOff() { gpio_set_level(dio2_gpio_, 0); }

    /** Установить GPIO для DIO2 (OOK DATA). Вызывать до initOOK(). */
    void setDio2Gpio(gpio_num_t gpio) { dio2_gpio_ = gpio; }

    uint8_t  readReg(uint8_t addr);
    void     writeReg(uint8_t addr, uint8_t value);
    void     setBits(uint8_t addr, uint8_t mask);
    void     clearBits(uint8_t addr, uint8_t mask);

private:
    void     setFrequency(uint32_t freq_hz);
    void     setMode(uint8_t mode); // 0=Sleep 1=Stby 2=FsTx 3=Tx 4=FsRx 5=RxCont 6=RxSingle 7=CAD
    void     readFifo(uint8_t *buf, uint8_t len);
    void     writeFifo(const uint8_t *buf, uint8_t len);

    static void IRAM_ATTR dio0IsrHandler(void *arg);

    spi_device_handle_t spi_dev_   = nullptr;
    gpio_num_t          dio0_gpio_ = GPIO_NUM_NC;
    gpio_num_t          dio2_gpio_ = GPIO_NUM_NC;
    RxCallback          rx_cb_;
    int8_t              last_rssi_ = 0;
    bool                lora_mode_ = true;
    volatile bool       irq_flag_  = false;
};

} // namespace lora
