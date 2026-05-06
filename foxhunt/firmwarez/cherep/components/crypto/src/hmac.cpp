#include "crypto/hmac.h"

#define MBEDTLS_DECLARE_PRIVATE_IDENTIFIERS
#include "mbedtls/md.h"
#include <cstring>

namespace crypto {

void hmac_sha256(const uint8_t *key, size_t key_len,
                 const uint8_t *data, size_t data_len,
                 uint8_t out[32])
{
    const mbedtls_md_info_t *md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    mbedtls_md_context_t ctx;
    mbedtls_md_init(&ctx);
    // md_setup (hmac=0) устанавливает ctx->md_info и аллоцирует md_ctx
    mbedtls_md_setup(&ctx, md_info, 0);
    // hmac_setup аллоцирует hmac_ctx (ipad/opad буферы)
    mbedtls_md_hmac_setup(&ctx, md_info);
    mbedtls_md_hmac_starts(&ctx, key, key_len);
    mbedtls_md_hmac_update(&ctx, data, data_len);
    mbedtls_md_hmac_finish(&ctx, out);
    mbedtls_md_free(&ctx);
}

void derive_sector_key(const uint8_t master_secret[16],
                       uint16_t event_id,
                       uint8_t out[6])
{
    uint8_t data[2] = {
        static_cast<uint8_t>(event_id >> 8),
        static_cast<uint8_t>(event_id & 0xFF)
    };
    uint8_t hash[32];
    hmac_sha256(master_secret, 16, data, 2, hash);
    memcpy(out, hash, 6);
}

void derive_station_key(const uint8_t master_secret[16],
                        uint8_t cp_num,
                        uint8_t out[16])
{
    uint8_t hash[32];
    hmac_sha256(master_secret, 16, &cp_num, 1, hash);
    memcpy(out, hash, 16);
}

void compute_cp_mac(const uint8_t station_key[16],
                    uint8_t cp_num,
                    uint64_t ts_arrive,
                    uint8_t flags,
                    uint16_t reserve,
                    const uint8_t *uid,
                    uint8_t uid_len,
                    uint8_t out[4])
{
    // Упаковка: cp_num(1) || ts_arrive(8 BE) || flags(1) || reserve(2 BE) || uid
    const size_t fixed_len = 1 + 8 + 1 + 2;
    uint8_t buf[fixed_len + 10]; // uid макс 10 байт
    size_t pos = 0;

    buf[pos++] = cp_num;

    buf[pos++] = static_cast<uint8_t>(ts_arrive >> 56);
    buf[pos++] = static_cast<uint8_t>(ts_arrive >> 48);
    buf[pos++] = static_cast<uint8_t>(ts_arrive >> 40);
    buf[pos++] = static_cast<uint8_t>(ts_arrive >> 32);
    buf[pos++] = static_cast<uint8_t>(ts_arrive >> 24);
    buf[pos++] = static_cast<uint8_t>(ts_arrive >> 16);
    buf[pos++] = static_cast<uint8_t>(ts_arrive >> 8);
    buf[pos++] = static_cast<uint8_t>(ts_arrive);

    buf[pos++] = flags;

    buf[pos++] = static_cast<uint8_t>(reserve >> 8);
    buf[pos++] = static_cast<uint8_t>(reserve & 0xFF);

    memcpy(buf + pos, uid, uid_len);
    pos += uid_len;

    uint8_t hash[32];
    hmac_sha256(station_key, 16, buf, pos, hash);
    memcpy(out, hash, 4);
}

void compute_participant_mac(const uint8_t master_secret[16],
                             const uint8_t *block1_data,
                             const uint8_t *name,
                             const uint8_t *uid,
                             uint8_t uid_len,
                             uint8_t out[4])
{
    // block1_data: первые 12 байт блока 1 (event_id[2]+pid[2]+gid[1]+fmt[1]+flags[1]+rsv[1]+rsv[4])
    // + name[16] + uid
    uint8_t buf[12 + 16 + 10];
    size_t pos = 0;

    memcpy(buf + pos, block1_data, 12);
    pos += 12;
    memcpy(buf + pos, name, 16);
    pos += 16;
    memcpy(buf + pos, uid, uid_len);
    pos += uid_len;

    uint8_t hash[32];
    hmac_sha256(master_secret, 16, buf, pos, hash);
    memcpy(out, hash, 4);
}

} // namespace crypto
