#include "lora/sx1276.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cstring>

static const char *TAG = "SX1276";

// Режимы RegOpMode (биты 2:0)
static constexpr uint8_t MODE_SLEEP    = 0x00;
static constexpr uint8_t MODE_STDBY    = 0x01;
static constexpr uint8_t MODE_TX       = 0x03;
static constexpr uint8_t MODE_RX_CONT  = 0x05;

// Флаги LongRangeMode и модуляция
static constexpr uint8_t LORA_FLAG     = 0x80; // LoRa mode
static constexpr uint8_t OOK_MOD       = 0x20; // OOK modulation type

namespace lora {

// ——— SPI ————————————————————————————————————————————————————————————————

uint8_t SX1276::readReg(uint8_t addr)
{
    uint8_t tx[2] = {(uint8_t)(addr & 0x7F), 0x00};
    uint8_t rx[2] = {};

    spi_transaction_t t = {};
    t.length    = 16;
    t.tx_buffer = tx;
    t.rx_buffer = rx;

    spi_device_transmit(spi_dev_, &t);
    return rx[1];
}

void SX1276::writeReg(uint8_t addr, uint8_t value)
{
    uint8_t tx[2] = {(uint8_t)(addr | 0x80), value};

    spi_transaction_t t = {};
    t.length    = 16;
    t.tx_buffer = tx;
    t.flags     = SPI_TRANS_USE_RXDATA;

    spi_device_transmit(spi_dev_, &t);
}

void SX1276::setBits(uint8_t addr, uint8_t mask)
{
    writeReg(addr, readReg(addr) | mask);
}

void SX1276::clearBits(uint8_t addr, uint8_t mask)
{
    writeReg(addr, readReg(addr) & ~mask);
}

// ——— Init ————————————————————————————————————————————————————————————————

esp_err_t SX1276::init(spi_host_device_t spi_host,
                       gpio_num_t cs_gpio,
                       gpio_num_t dio0_gpio,
                       gpio_num_t rst_gpio)
{
    dio0_gpio_ = dio0_gpio;

    // Hardware reset
    if (rst_gpio != GPIO_NUM_NC) {
        gpio_set_direction(rst_gpio, GPIO_MODE_OUTPUT);
        gpio_set_level(rst_gpio, 0);
        vTaskDelay(pdMS_TO_TICKS(10));
        gpio_set_level(rst_gpio, 1);
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    spi_device_interface_config_t devcfg = {};
    devcfg.clock_speed_hz = 4 * 1000 * 1000; // 4 МГц
    devcfg.mode           = 0;
    devcfg.spics_io_num   = cs_gpio;
    devcfg.queue_size     = 4;

    esp_err_t ret = spi_bus_add_device(spi_host, &devcfg, &spi_dev_);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI add device failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // Проверить версию
    uint8_t ver = readReg(Reg::Version);
    ESP_LOGI(TAG, "SX1276 version: 0x%02X", ver);
    if (ver != 0x12) {
        ESP_LOGW(TAG, "Expected 0x12, got 0x%02X", ver);
    }

    // Настроить DIO0 как input с ISR
    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << dio0_gpio);
    io_conf.mode         = GPIO_MODE_INPUT;
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    io_conf.intr_type    = GPIO_INTR_POSEDGE;
    gpio_config(&io_conf);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(dio0_gpio, dio0IsrHandler, this);

    return ESP_OK;
}

// ——— DIO0 ISR ————————————————————————————————————————————————————————————

void IRAM_ATTR SX1276::dio0IsrHandler(void *arg)
{
    SX1276 *self = static_cast<SX1276 *>(arg);
    self->irq_flag_ = true;
}

void SX1276::handleDio0Isr()
{
    if (!irq_flag_) return;
    irq_flag_ = false;

    uint8_t irq = readReg(Reg::IrqFlags);
    writeReg(Reg::IrqFlags, irq); // сброс флагов

    if (irq & IRQ_RX_DONE) {
        if (irq & IRQ_CRC_ERR) {
            ESP_LOGW(TAG, "RX CRC error");
            return;
        }

        uint8_t len  = readReg(Reg::RxNbBytes);
        uint8_t addr = readReg(Reg::FifoRxCurrentAddr);
        writeReg(Reg::FifoAddrPtr, addr);

        uint8_t buf[255];
        readFifo(buf, len);

        int8_t snr  = static_cast<int8_t>(readReg(Reg::PktSnrValue)) / 4;
        last_rssi_  = -157 + readReg(Reg::PktRssiValue); // для 433 МГц

        ESP_LOGI(TAG, "RX: %d bytes, RSSI=%d SNR=%d", len, last_rssi_, snr);

        if (rx_cb_) rx_cb_(buf, len, last_rssi_);
    } else if (irq & IRQ_TX_DONE) {
        ESP_LOGD(TAG, "TX done");
    }
}

// ——— Установка частоты ———————————————————————————————————————————————————

void SX1276::setFrequency(uint32_t freq_hz)
{
    // Frf = freq_hz * 2^19 / 32000000
    uint64_t frf = ((uint64_t)freq_hz << 19) / 32000000UL;
    writeReg(Reg::FrfMsb, (uint8_t)(frf >> 16));
    writeReg(Reg::FrfMid, (uint8_t)(frf >> 8));
    writeReg(Reg::FrfLsb, (uint8_t)(frf));
}

void SX1276::setMode(uint8_t mode)
{
    uint8_t cur = readReg(Reg::OpMode);
    writeReg(Reg::OpMode, (cur & 0xF8) | (mode & 0x07));
    vTaskDelay(pdMS_TO_TICKS(1));
}

// ——— LoRa режим ——————————————————————————————————————————————————————————

esp_err_t SX1276::initLoRa(uint32_t freq_hz, uint8_t sf,
                            uint16_t bw_khz, uint8_t cr)
{
    lora_mode_ = true;

    // Sleep mode → сменить на LoRa
    writeReg(Reg::OpMode, MODE_SLEEP);
    vTaskDelay(pdMS_TO_TICKS(10));
    writeReg(Reg::OpMode, LORA_FLAG | MODE_SLEEP);
    vTaskDelay(pdMS_TO_TICKS(10));

    setFrequency(freq_hz);

    // PA: +17 dBm через PA_BOOST
    writeReg(Reg::PaConfig, 0x8F); // PA_BOOST, MaxPower=7, OutputPower=15
    writeReg(Reg::PaDac,    0x84); // стандартный режим

    // Защита от перегрева
    writeReg(Reg::Ocp, 0x3B); // OcpOn=1, OcpTrim=11 → 140mA

    // LNA: максимальное усиление
    writeReg(Reg::Lna, 0x23); // LnaGain=1 (max), LnaBoostHf=3

    // ModemConfig1: BW + CR + implicit header
    uint8_t bw_bits;
    if      (bw_khz >= 500) bw_bits = 0x09;
    else if (bw_khz >= 250) bw_bits = 0x08;
    else                    bw_bits = 0x07; // 125 кГц

    uint8_t cr_bits = ((cr - 1) & 0x03) + 1; // 1-4 → 1-4
    writeReg(Reg::ModemConfig1, (bw_bits << 4) | (cr_bits << 1) | 0x00);

    // ModemConfig2: SF + TxContinuous=0 + RxPayloadCrcOn=1
    if (sf < 6) sf = 6;
    if (sf > 12) sf = 12;
    writeReg(Reg::ModemConfig2, (sf << 4) | 0x04); // RxCRC on

    // SF6 оптимизация
    if (sf == 6) {
        writeReg(Reg::DetectOptimize,  0xC5);
        writeReg(Reg::DetectionThres,  0x0C);
    } else {
        writeReg(Reg::DetectOptimize,  0xC3);
        writeReg(Reg::DetectionThres,  0x0A);
    }

    // ModemConfig3: LowDataRateOptimize + AGC
    writeReg(Reg::ModemConfig3, 0x04); // AgcAutoOn=1

    // Sync word: 0x12 для приватной сети
    writeReg(Reg::SyncWord, 0x12);

    // Преамбула: 8 символов
    writeReg(Reg::PreambleMsb, 0x00);
    writeReg(Reg::PreambleLsb, 0x08);

    // FIFO базовые адреса
    writeReg(Reg::FifoTxBaseAddr, 0x00);
    writeReg(Reg::FifoRxBaseAddr, 0x00);

    // DIO0 → RxDone / TxDone
    writeReg(Reg::DioMapping1, 0x00); // DIO0=00 (RxDone), DIO0=01 (TxDone)

    // Перейти в Standby
    writeReg(Reg::OpMode, LORA_FLAG | MODE_STDBY);
    vTaskDelay(pdMS_TO_TICKS(10));

    ESP_LOGI(TAG, "LoRa init: %lu Hz SF%d BW%d CR4/%d",
             (unsigned long)freq_hz, sf, bw_khz, cr + 4);
    return ESP_OK;
}

// ——— FSK режим (Fox — морзянка тоном по FM) ———————————————————————————————
//
// SX1276 в FSK TX continuous режиме. GPIO DIO2 = прямой вход модуляции:
//   DIO2=0 → несущая f0 - Fdev (тишина на FM-приёмнике)
//   DIO2=1 → несущая f0 + Fdev
// Меандр на DIO2 с частотой tone_hz → FM-демодулятор воспроизводит аудио тон.
//
// Параметры:
//   fdev_hz = 3500 Гц — в пределах NFM стандарта ±5 кГц, даёт хороший уровень
//   FSTEP = Fxosc/2^19 = 32000000/524288 ≈ 61.035 Гц
//   Fdev_reg = round(fdev_hz / 61.035)

esp_err_t SX1276::initFSK(uint32_t freq_hz, uint32_t fdev_hz)
{
    lora_mode_ = false;

    // Перейти в Sleep в FSK режиме.
    // LongRangeMode можно менять только из Sleep — сначала входим в LoRa Sleep,
    // затем сбрасываем LongRangeMode (переходим в FSK Sleep).
    writeReg(Reg::OpMode, LORA_FLAG | MODE_SLEEP); // LoRa Sleep
    vTaskDelay(pdMS_TO_TICKS(10));
    writeReg(Reg::OpMode, MODE_SLEEP);             // FSK Sleep (LongRangeMode=0)
    vTaskDelay(pdMS_TO_TICKS(10));

    setFrequency(freq_hz);

    // Девиация частоты: Fdev_reg = Fdev_hz / FSTEP
    uint32_t fdev_reg = (fdev_hz * 524288ULL + 16000000UL) / 32000000UL;
    writeReg(Reg::FdevMsb, (uint8_t)(fdev_reg >> 8));
    writeReg(Reg::FdevLsb, (uint8_t)(fdev_reg & 0xFF));

    // Bitrate: 4800 bps (не используется в continuous direct mode, но нужно задать)
    // BR_reg = Fxosc / BR = 32000000 / 4800 = 6666 = 0x1A0A
    writeReg(Reg::Bitrate_Msb, 0x1A);
    writeReg(Reg::Bitrate_Lsb, 0x0A);

    // PA: +17 dBm через PA_BOOST
    writeReg(Reg::PaConfig, 0x8F);
    writeReg(Reg::PaDac,    0x84);

    // DataMode = 0 (continuous) — RegPacketConfig2 [6] = 0
    // Нужно для прямой модуляции через DIO2
    writeReg(Reg::PacketConfig2, readReg(Reg::PacketConfig2) & ~0x40);

    // DIO2 → DATA (прямая модуляция в continuous TX режиме)
    // RegDioMapping1 bits [3:2] = 01 (DATA)
    writeReg(Reg::DioMapping1, 0x04);

    // GPIO DIO2 как output
    if (dio2_gpio_ != GPIO_NUM_NC) {
        gpio_set_direction(dio2_gpio_, GPIO_MODE_OUTPUT);
        gpio_set_level(dio2_gpio_, 0);
    }

    // Standby
    writeReg(Reg::OpMode, MODE_STDBY);
    vTaskDelay(pdMS_TO_TICKS(5));

    ESP_LOGI(TAG, "FSK init: %lu Hz, Fdev=%lu Hz (reg=0x%04lX)",
             (unsigned long)freq_hz, (unsigned long)fdev_hz, (unsigned long)fdev_reg);
    return ESP_OK;
}

// ——— sendPacket (LoRa TX) ————————————————————————————————————————————————

esp_err_t SX1276::sendPacket(const uint8_t *data, uint8_t len)
{
    // Standby
    writeReg(Reg::OpMode, LORA_FLAG | MODE_STDBY);
    vTaskDelay(pdMS_TO_TICKS(1));

    // DIO0 → TxDone
    writeReg(Reg::DioMapping1, 0x40); // DIO0=01 → TxDone

    // Сброс IRQ
    writeReg(Reg::IrqFlags, 0xFF);

    // FIFO
    writeReg(Reg::FifoAddrPtr, 0x00);
    writeFifo(data, len);
    writeReg(Reg::PayloadLength, len);

    // TX
    writeReg(Reg::OpMode, LORA_FLAG | MODE_TX);

    // Ждём TxDone через polling (до 2 секунд)
    uint32_t timeout_ms = 2000;
    while (timeout_ms--) {
        uint8_t irq = readReg(Reg::IrqFlags);
        if (irq & IRQ_TX_DONE) {
            writeReg(Reg::IrqFlags, IRQ_TX_DONE);
            // Восстановить DIO0 для RxDone
            writeReg(Reg::DioMapping1, 0x00);
            return ESP_OK;
        }
        vTaskDelay(1);
    }

    ESP_LOGE(TAG, "TX timeout");
    writeReg(Reg::DioMapping1, 0x00);
    return ESP_ERR_TIMEOUT;
}

// ——— startReceive ————————————————————————————————————————————————————————

void SX1276::startReceive()
{
    writeReg(Reg::IrqFlags,     0xFF);
    writeReg(Reg::FifoAddrPtr,  0x00);
    writeReg(Reg::DioMapping1,  0x00); // DIO0=00 → RxDone
    writeReg(Reg::OpMode, LORA_FLAG | MODE_RX_CONT);
    ESP_LOGD(TAG, "RX continuous started");
}

// ——— FIFO access ————————————————————————————————————————————————————————

void SX1276::readFifo(uint8_t *buf, uint8_t len)
{
    // SX1276 FIFO burst read: addr=0x00 → read bytes
    // Максимум 256 байт FIFO + 1 адресный байт
    static constexpr uint16_t MAX_FIFO = 256;
    if (len > MAX_FIFO) len = (uint8_t)MAX_FIFO;

    uint8_t tx_buf[MAX_FIFO + 1] = {};
    uint8_t rx_buf[MAX_FIFO + 1] = {};
    tx_buf[0] = Reg::Fifo & 0x7F; // read addr

    spi_transaction_t t = {};
    t.length    = (len + 1) * 8;
    t.tx_buffer = tx_buf;
    t.rx_buffer = rx_buf;

    spi_device_transmit(spi_dev_, &t);
    memcpy(buf, rx_buf + 1, len);
}

void SX1276::writeFifo(const uint8_t *buf, uint8_t len)
{
    static constexpr uint16_t MAX_FIFO = 256;
    uint8_t tx_buf[MAX_FIFO + 1];
    tx_buf[0] = Reg::Fifo | 0x80; // write addr
    if (len > MAX_FIFO) len = (uint8_t)MAX_FIFO;
    memcpy(tx_buf + 1, buf, len);

    spi_transaction_t t = {};
    t.length    = (len + 1) * 8;
    t.tx_buffer = tx_buf;
    t.flags     = SPI_TRANS_USE_RXDATA;

    spi_device_transmit(spi_dev_, &t);
}

} // namespace lora
