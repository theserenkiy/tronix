#pragma once

#include <cstdint>
#include <cstring>

namespace config {

enum class TaskType : uint8_t {
    NONE       = 0,
    ALCO_TEST  = 1,
    // Зарезервировано для будущих типов
};

struct ExtraTaskConfig {
    TaskType  type        = TaskType::NONE;
    uint16_t  bonus_score = 0;
    // reserve_pass = 1 для pass_fail типа
};

struct StationConfig {
    uint8_t  cp_num        = 0xFF;       // 0xFF = не инициализирован
    uint16_t event_id      = 0;
    uint8_t  station_key[16] = {};       // HMAC-SHA256(master_secret, cp_num)[0:16]
    uint8_t  sector_key[6]   = {};       // HMAC-SHA256(master_secret, event_id_be)[0:6]

    bool     is_fox        = false;
    uint32_t fox_on_sec    = 15;         // секунд передачи
    uint32_t fox_off_sec   = 60;         // секунд паузы
    char     fox_morse[16] = "MO1";      // Morse-строка

    bool     has_task      = false;
    ExtraTaskConfig task;

    uint32_t lora_freq_hz  = 433175000;  // рабочая частота LoRa
    uint32_t fox_freq_hz   = 433920000;  // несущая частота Fox FSK
    uint32_t fox_fdev_hz   = 3500;       // девиация Fox FSK, Гц
    uint32_t fox_tone_hz   = 800;        // частота аудио тона морзянки, Гц

    bool     initialized   = false;      // конфиг был записан хотя бы раз
    uint32_t last_timestamp = 0;         // последний известный unix-timestamp (для восстановления после ребута)

    bool isValid() const {
        return initialized && (cp_num != 0xFF) && (event_id != 0);
    }
};

} // namespace config
