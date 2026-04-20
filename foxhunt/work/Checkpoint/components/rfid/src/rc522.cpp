#include "rfid/rc522.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cstring>

static const char *TAG = "RC522";

namespace rfid {

// ——— SPI адресный байт ————————————————————————————————————————————————————
// Чтение:  (addr << 1) | 0x80
// Запись:  (addr << 1) & 0xFE
static inline uint8_t read_addr(uint8_t reg)  { return ((reg << 1) & 0x7E) | 0x80; }
static inline uint8_t write_addr(uint8_t reg) { return  (reg << 1) & 0x7E; }

// ——— Init ————————————————————————————————————————————————————————————————

esp_err_t RC522::init(spi_host_device_t spi_host,
                      gpio_num_t cs_gpio,
                      gpio_num_t rst_gpio)
{
    rst_gpio_ = rst_gpio;

    // Аппаратный reset если пин задан
    if (rst_gpio_ != GPIO_NUM_NC) {
        gpio_set_direction(rst_gpio_, GPIO_MODE_OUTPUT);
        gpio_set_level(rst_gpio_, 0);
        vTaskDelay(pdMS_TO_TICKS(10));
        gpio_set_level(rst_gpio_, 1);
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    spi_device_interface_config_t devcfg = {};
    devcfg.clock_speed_hz = 5 * 1000 * 1000; // 5 МГц
    devcfg.mode           = 0;                // CPOL=0, CPHA=0
    devcfg.spics_io_num   = cs_gpio;
    devcfg.queue_size     = 4;

    esp_err_t ret = spi_bus_add_device(spi_host, &devcfg, &spi_dev_);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add SPI device: %s", esp_err_to_name(ret));
        return ret;
    }

    // Программный сброс
    reset();

    // Таймер: автоматически запускать таймер, fTimer = 13.56 МГц / (2*TPrescaler+2)
    // TPrescaler = 0x0A9 → fTimer ≈ 40 кГц
    writeReg(TModeReg,      0x80); // TAuto=1
    writeReg(TPrescalerReg, 0xA9);
    writeReg(TReloadRegH,   0x03);
    writeReg(TReloadRegL,   0xE8);

    writeReg(TxASKReg, 0x40); // ForceASK100%=1
    writeReg(ModeReg,  0x3D); // CRC preset = 6363

    // Включить антенну
    setBits(TxControlReg, 0x03);

    uint8_t ver = getVersion();
    ESP_LOGI(TAG, "RC522 version: 0x%02X", ver);
    if (ver != 0x91 && ver != 0x92) {
        ESP_LOGW(TAG, "Unexpected RC522 version (may still work)");
    }

    return ESP_OK;
}

void RC522::reset()
{
    writeReg(CommandReg, PCD_SoftReset);
    // Ждём завершения reset (бит PowerDown сбрасывается)
    uint32_t timeout = 50;
    while ((readReg(CommandReg) & 0x10) && timeout--) {
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

// ——— Регистровый доступ ——————————————————————————————————————————————————

uint8_t RC522::readReg(PcdReg reg)
{
    uint8_t tx[2] = {read_addr(reg), 0x00};
    uint8_t rx[2] = {};

    spi_transaction_t t = {};
    t.length    = 16;
    t.tx_buffer = tx;
    t.rx_buffer = rx;

    spi_device_transmit(spi_dev_, &t);
    return rx[1];
}

void RC522::writeReg(PcdReg reg, uint8_t value)
{
    uint8_t tx[2] = {write_addr(reg), value};

    spi_transaction_t t = {};
    t.length    = 16;
    t.tx_buffer = tx;
    t.flags     = SPI_TRANS_USE_RXDATA; // игнорировать rx

    spi_device_transmit(spi_dev_, &t);
}

void RC522::setBits(PcdReg reg, uint8_t mask)
{
    writeReg(reg, readReg(reg) | mask);
}

void RC522::clearBits(PcdReg reg, uint8_t mask)
{
    writeReg(reg, readReg(reg) & ~mask);
}

// ——— FIFO ————————————————————————————————————————————————————————————————

void RC522::flushFifo()
{
    setBits(FIFOLevelReg, 0x80);
}

void RC522::writeFifo(const uint8_t *data, uint8_t len)
{
    for (uint8_t i = 0; i < len; i++) {
        writeReg(FIFODataReg, data[i]);
    }
}

void RC522::readFifo(uint8_t *data, uint8_t len)
{
    if (len == 0) return;

    // Для мульти-байтового чтения посылаем адрес + len-1 повторений + 0x00
    // Макс 64 байта из FIFO RC522
    static constexpr uint8_t MAX_FIFO = 64;
    if (len > MAX_FIFO) len = MAX_FIFO;

    uint8_t tx_buf[MAX_FIFO + 1];
    uint8_t rx_buf[MAX_FIFO + 1];

    tx_buf[0] = read_addr(FIFODataReg);
    for (uint8_t i = 1; i < len; i++) {
        tx_buf[i] = read_addr(FIFODataReg);
    }
    tx_buf[len] = 0x00; // последний dummy

    spi_transaction_t t = {};
    t.length    = (len + 1) * 8;
    t.tx_buffer = tx_buf;
    t.rx_buffer = rx_buf;

    spi_device_transmit(spi_dev_, &t);

    // Данные начинаются с rx_buf[1]
    for (uint8_t i = 0; i < len; i++) {
        data[i] = rx_buf[i + 1];
    }
}

// ——— CRC ——————————————————————————————————————————————————————————————————

Status RC522::calcCRC(const uint8_t *data, uint8_t len, uint8_t result[2])
{
    writeReg(CommandReg, PCD_Idle);
    clearBits(DivIrqReg, 0x04); // CRCIrq = 0
    flushFifo();
    writeFifo(data, len);
    writeReg(CommandReg, PCD_CalcCRC);

    uint32_t deadline = 200; // ~200ms таймаут
    while (deadline--) {
        uint8_t irq = readReg(DivIrqReg);
        if (irq & 0x04) {
            result[0] = readReg(CRCResultRegL);
            result[1] = readReg(CRCResultRegH);
            return Status::OK;
        }
        vTaskDelay(1);
    }
    return Status::TIMEOUT;
}

// ——— Transceive ————————————————————————————————————————————————————————

Status RC522::communicate(PcdCmd cmd,
                          const uint8_t *send_data, uint8_t send_len,
                          uint8_t *recv_data, uint8_t *recv_len,
                          uint8_t *valid_bits,
                          uint8_t rx_align,
                          bool check_crc)
{
    uint8_t valid_bits_tx = valid_bits ? *valid_bits : 0;
    uint8_t wait_irq = (cmd == PCD_MFAuthent) ? 0x10 : 0x30;
    // 0x10 = IdleIRq, 0x30 = IdleIRq | RxIRq

    writeReg(CommandReg,    PCD_Idle);
    clearBits(ComIrqReg,    0x80);    // сброс всех IRQ
    flushFifo();
    writeFifo(send_data, send_len);
    writeReg(CommandReg,    cmd);

    if (cmd == PCD_Transceive) {
        setBits(BitFramingReg, 0x80); // StartSend=1
    }

    // Ждём завершения
    uint32_t deadline = 40; // ~40ms
    bool completed = false;
    while (deadline--) {
        uint8_t irq = readReg(ComIrqReg);
        if (irq & wait_irq) {
            completed = true;
            break;
        }
        if (irq & 0x01) { // TimerIRq
            break;
        }
        vTaskDelay(1);
    }

    clearBits(BitFramingReg, 0x80); // StartSend=0

    if (!completed) return Status::TIMEOUT;

    uint8_t error_reg = readReg(ErrorReg);
    if (error_reg & 0x13) { // BufferOvfl | ParityErr | ProtocolErr
        return Status::ERROR;
    }
    if (error_reg & 0x08) { // CollErr
        return Status::COLLISION;
    }

    if (recv_data && recv_len) {
        uint8_t n = readReg(FIFOLevelReg);
        if (n > *recv_len) {
            return Status::NO_ROOM;
        }
        *recv_len = n;

        uint8_t ctrl = readReg(ControlReg);
        uint8_t rx_last_bits = ctrl & 0x07;
        if (valid_bits) *valid_bits = rx_last_bits;

        if (rx_align) {
            // Выравнивание первого байта
            uint8_t mask = (0xFF << rx_align) & 0xFF;
            recv_data[0] = (readReg(FIFODataReg) & mask) |
                           (recv_data[0] & ~mask);
            if (n > 1) {
                readFifo(recv_data + 1, n - 1);
            }
        } else {
            readFifo(recv_data, n);
        }

        if (check_crc && n >= 2) {
            if (rx_last_bits != 0) return Status::CRC_WRONG;
            uint8_t crc[2];
            if (calcCRC(recv_data, n - 2, crc) != Status::OK) {
                return Status::TIMEOUT;
            }
            if (crc[0] != recv_data[n-2] || crc[1] != recv_data[n-1]) {
                return Status::CRC_WRONG;
            }
        }
    }

    // Проверить NACK (1 байт = 0x00 для MiFARE NACK)
    if (recv_data && recv_len && *recv_len == 1) {
        uint8_t vb = valid_bits ? *valid_bits : 0;
        if (vb == 4 && recv_data[0] == 0x00) {
            return Status::MIFARE_NACK;
        }
    }

    return Status::OK;
}

} // namespace rfid
