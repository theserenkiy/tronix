#pragma once

#include <cstdint>
#include <cstddef>

namespace crypto {

/**
 * Вычислить HMAC-SHA256.
 * @param key      ключ
 * @param key_len  длина ключа
 * @param data     данные
 * @param data_len длина данных
 * @param out      выходной буфер, 32 байта
 */
void hmac_sha256(const uint8_t *key, size_t key_len,
                 const uint8_t *data, size_t data_len,
                 uint8_t out[32]);

/**
 * Вывести ключ сектора: HMAC-SHA256(master_secret, event_id_be)[0:6]
 * event_id передаётся как uint16_t, преобразуется в 2 байта BE внутри функции.
 */
void derive_sector_key(const uint8_t master_secret[16],
                       uint16_t event_id,
                       uint8_t out[6]);

/**
 * Вывести ключ станции: HMAC-SHA256(master_secret, cp_num)[0:16]
 */
void derive_station_key(const uint8_t master_secret[16],
                        uint8_t cp_num,
                        uint8_t out[16]);

/**
 * Вычислить MAC блока КП:
 * HMAC-SHA256(station_key, cp_num||ts_arrive_be||flags||reserve_be||uid)[0:4]
 * ts_arrive и reserve упаковываются в BE внутри функции.
 */
void compute_cp_mac(const uint8_t station_key[16],
                    uint8_t cp_num,
                    uint64_t ts_arrive,
                    uint8_t flags,
                    uint16_t reserve,
                    const uint8_t *uid,
                    uint8_t uid_len,
                    uint8_t out[4]);

/**
 * Вычислить MAC блока участника (сектор 0, блок 1):
 * HMAC-SHA256(master_secret,
 *   event_id||participant_id||group_id||format||flags||reserve||name||UID)[0:4]
 * Все поля передаются как есть (caller упаковывает BE).
 */
void compute_participant_mac(const uint8_t master_secret[16],
                             const uint8_t *block1_data, // байты 0..11
                             const uint8_t *name,        // 16 байт из блока 2
                             const uint8_t *uid,
                             uint8_t uid_len,
                             uint8_t out[4]);

} // namespace crypto
