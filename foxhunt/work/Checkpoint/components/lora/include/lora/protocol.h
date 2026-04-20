#pragma once

#include "lora/sx1276.h"
#include <cstdint>
#include <cstddef>

namespace lora {

// ——— Протокол пакетов LoRa с базовой станцией ————————————————————————————
// Формат: [MAGIC:1][TYPE:1][EVENT_ID:2][CP_NUM:1][SEQ:2][LEN:1][PAYLOAD:N][CRC16:2]
// Итого хедер: 9 байт, макс payload: 255-11 = 244 байта

static constexpr uint8_t PROTO_MAGIC = 0xFA;

enum class PacketType : uint8_t {
    REQ_LOG     = 0x01, // Базовая → КП: запрос лога посещений
    RESP_LOG    = 0x02, // КП → Базовая: ответ с записями лога
    REQ_STATUS  = 0x03, // Базовая → КП: запрос статуса
    RESP_STATUS = 0x04, // КП → Базовая: статус
    TIME_SYNC   = 0x05, // Базовая → КП: синхронизация времени
    ACK         = 0x06, // Подтверждение
};

#pragma pack(push, 1)

struct PacketHeader {
    uint8_t  magic;
    uint8_t  type;
    uint16_t event_id;    // Big Endian
    uint8_t  cp_num;
    uint16_t seq;         // Big Endian
    uint8_t  payload_len;
};

// Запись лога для передачи по LoRa
struct LogEntryPacket {
    uint8_t  uid[4];
    uint16_t participant_id; // BE
    uint8_t  cp_num;
    uint64_t ts_arrive;      // BE
    uint8_t  flags;
    uint16_t reserve;        // BE
};

struct StatusPacket {
    uint8_t  cp_num;
    uint8_t  is_fox;
    uint16_t log_count;  // BE
    uint32_t uptime_sec; // BE
    int8_t   rssi_last;
};

struct TimeSyncPayload {
    uint64_t unix_ts; // BE
};

#pragma pack(pop)

// CRC-16/CCITT-FALSE (polynomial 0x1021, init 0xFFFF)
uint16_t crc16(const uint8_t *data, size_t len);

/**
 * Протокол связи КП с базовой станцией.
 * Обрабатывает входящие пакеты и отправляет ответы.
 */
class LoRaProtocol {
public:
    /**
     * @param radio    SX1276 драйвер
     * @param event_id ID соревнования
     * @param cp_num   Номер КП
     */
    LoRaProtocol(SX1276 &radio, uint16_t event_id, uint8_t cp_num);

    /** Установить callback для получения лога (при REQ_LOG запрос логи через этот cb). */
    using LogProvider = std::function<size_t(LogEntryPacket *buf, size_t max_entries)>;
    void setLogProvider(LogProvider lp) { log_provider_ = lp; }

    /** Вызывать периодически для обработки принятых пакетов. */
    void process(const uint8_t *data, uint8_t len, int8_t rssi);

    /** Отправить статусный пакет. */
    esp_err_t sendStatus(uint16_t log_count, uint32_t uptime_sec);

private:
    esp_err_t handleReqLog(const uint8_t *payload, uint8_t plen);
    esp_err_t handleReqStatus(const uint8_t *payload, uint8_t plen);
    esp_err_t handleTimeSync(const uint8_t *payload, uint8_t plen);
    esp_err_t sendPacket(PacketType type, const uint8_t *payload, uint8_t plen);

    bool parsePacket(const uint8_t *data, uint8_t len,
                     PacketHeader &hdr, const uint8_t **payload);

    SX1276      &radio_;
    uint16_t     event_id_;
    uint8_t      cp_num_;
    uint16_t     tx_seq_ = 0;
    LogProvider  log_provider_;
};

} // namespace lora
