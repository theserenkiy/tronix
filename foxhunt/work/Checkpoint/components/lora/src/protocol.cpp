#include "lora/protocol.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <cstring>
#include <sys/time.h>

static const char *TAG = "LoRaProto";

namespace lora {

// ——— CRC-16/CCITT-FALSE ——————————————————————————————————————————————————

uint16_t crc16(const uint8_t *data, size_t len)
{
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (int j = 0; j < 8; j++) {
            if (crc & 0x8000) crc = (crc << 1) ^ 0x1021;
            else              crc <<= 1;
        }
    }
    return crc;
}

// ——— Constructor ————————————————————————————————————————————————————————

LoRaProtocol::LoRaProtocol(SX1276 &radio, uint16_t event_id, uint8_t cp_num)
    : radio_(radio), event_id_(event_id), cp_num_(cp_num)
{}

// ——— Разбор пакета ——————————————————————————————————————————————————————

bool LoRaProtocol::parsePacket(const uint8_t *data, uint8_t len,
                                PacketHeader &hdr, const uint8_t **payload)
{
    if (len < sizeof(PacketHeader) + 2) return false; // хедер + CRC

    if (data[0] != PROTO_MAGIC) {
        ESP_LOGW(TAG, "Bad magic: 0x%02X", data[0]);
        return false;
    }

    memcpy(&hdr, data, sizeof(PacketHeader));
    hdr.event_id = (data[2] << 8) | data[3]; // BE
    hdr.seq      = (data[5] << 8) | data[6]; // BE

    uint8_t payload_len = hdr.payload_len;
    if ((size_t)(sizeof(PacketHeader) + payload_len + 2) > len) {
        ESP_LOGW(TAG, "Packet too short: expected %zu, got %d",
                 sizeof(PacketHeader) + payload_len + 2, len);
        return false;
    }

    // Проверить CRC
    size_t crc_offset = sizeof(PacketHeader) + payload_len;
    uint16_t pkt_crc  = (data[crc_offset] << 8) | data[crc_offset + 1];
    uint16_t calc_crc = crc16(data, crc_offset);

    if (pkt_crc != calc_crc) {
        ESP_LOGW(TAG, "CRC mismatch: 0x%04X != 0x%04X", pkt_crc, calc_crc);
        return false;
    }

    *payload = data + sizeof(PacketHeader);
    return true;
}

// ——— Отправка пакета ————————————————————————————————————————————————————

esp_err_t LoRaProtocol::sendPacket(PacketType type,
                                    const uint8_t *payload, uint8_t plen)
{
    uint8_t buf[9 + 244 + 2]; // хедер + макс payload + CRC
    size_t  pos = 0;

    buf[pos++] = PROTO_MAGIC;
    buf[pos++] = static_cast<uint8_t>(type);
    buf[pos++] = (event_id_ >> 8) & 0xFF; // BE
    buf[pos++] = event_id_ & 0xFF;
    buf[pos++] = cp_num_;
    buf[pos++] = (tx_seq_ >> 8) & 0xFF;
    buf[pos++] = tx_seq_ & 0xFF;
    buf[pos++] = plen;

    if (plen > 0 && payload) {
        memcpy(buf + pos, payload, plen);
        pos += plen;
    }

    uint16_t crc = crc16(buf, pos);
    buf[pos++] = (crc >> 8) & 0xFF;
    buf[pos++] = crc & 0xFF;

    tx_seq_++;
    return radio_.sendPacket(buf, (uint8_t)pos);
}

// ——— process ————————————————————————————————————————————————————————————

void LoRaProtocol::process(const uint8_t *data, uint8_t len, int8_t rssi)
{
    PacketHeader hdr;
    const uint8_t *payload;

    if (!parsePacket(data, len, hdr, &payload)) return;

    // Фильтровать по event_id и cp_num
    if (hdr.event_id != event_id_) {
        ESP_LOGD(TAG, "Wrong event_id: %d", hdr.event_id);
        return;
    }
    if (hdr.cp_num != cp_num_ && hdr.cp_num != 0xFF) {
        ESP_LOGD(TAG, "Wrong cp_num: %d", hdr.cp_num);
        return;
    }

    PacketType type = static_cast<PacketType>(hdr.type);
    ESP_LOGI(TAG, "Rx pkt type=0x%02X seq=%d len=%d rssi=%d",
             hdr.type, hdr.seq, hdr.payload_len, rssi);

    switch (type) {
    case PacketType::REQ_LOG:
        handleReqLog(payload, hdr.payload_len);
        break;
    case PacketType::REQ_STATUS:
        handleReqStatus(payload, hdr.payload_len);
        break;
    case PacketType::TIME_SYNC:
        handleTimeSync(payload, hdr.payload_len);
        break;
    default:
        ESP_LOGD(TAG, "Unhandled packet type: 0x%02X", hdr.type);
        break;
    }
}

// ——— handleReqLog ————————————————————————————————————————————————————————

esp_err_t LoRaProtocol::handleReqLog(const uint8_t *payload, uint8_t plen)
{
    if (!log_provider_) {
        ESP_LOGW(TAG, "No log provider set");
        return ESP_FAIL;
    }

    // Получить записи лога
    const size_t max_entries = 8; // макс в одном пакете (8 * 18 = 144 байт)
    LogEntryPacket entries[max_entries];
    size_t count = log_provider_(entries, max_entries);

    if (count == 0) {
        // Пустой ответ
        return sendPacket(PacketType::RESP_LOG, nullptr, 0);
    }

    // Упаковать в payload (BE)
    uint8_t buf[max_entries * sizeof(LogEntryPacket) + 1];
    size_t  pos = 0;

    buf[pos++] = (uint8_t)count;

    for (size_t i = 0; i < count; i++) {
        const LogEntryPacket &e = entries[i];
        memcpy(buf + pos, e.uid, 4); pos += 4;

        buf[pos++] = (e.participant_id >> 8) & 0xFF;
        buf[pos++] = e.participant_id & 0xFF;

        buf[pos++] = e.cp_num;

        for (int b = 7; b >= 0; b--) {
            buf[pos++] = (uint8_t)(e.ts_arrive >> (b * 8));
        }

        buf[pos++] = e.flags;
        buf[pos++] = (e.reserve >> 8) & 0xFF;
        buf[pos++] = e.reserve & 0xFF;
    }

    return sendPacket(PacketType::RESP_LOG, buf, (uint8_t)pos);
}

// ——— handleReqStatus ————————————————————————————————————————————————————

esp_err_t LoRaProtocol::handleReqStatus(const uint8_t *payload, uint8_t plen)
{
    return sendStatus(0, esp_timer_get_time() / 1000000ULL);
}

esp_err_t LoRaProtocol::sendStatus(uint16_t log_count, uint32_t uptime_sec)
{
    StatusPacket sp;
    sp.cp_num     = cp_num_;
    sp.is_fox     = 0;
    sp.log_count  = __builtin_bswap16(log_count);
    sp.uptime_sec = __builtin_bswap32(uptime_sec);
    sp.rssi_last  = radio_.lastRssi();

    return sendPacket(PacketType::RESP_STATUS,
                      reinterpret_cast<uint8_t *>(&sp), sizeof(sp));
}

// ——— handleTimeSync ——————————————————————————————————————————————————————

esp_err_t LoRaProtocol::handleTimeSync(const uint8_t *payload, uint8_t plen)
{
    if (plen < 8) return ESP_ERR_INVALID_SIZE;

    uint64_t ts = 0;
    for (int i = 0; i < 8; i++) {
        ts = (ts << 8) | payload[i];
    }

    struct timeval tv = { .tv_sec = (time_t)ts, .tv_usec = 0 };
    settimeofday(&tv, nullptr);
    ESP_LOGI(TAG, "Time synced via LoRa: %" PRIu64, ts);

    // ACK
    return sendPacket(PacketType::ACK, nullptr, 0);
}

} // namespace lora
