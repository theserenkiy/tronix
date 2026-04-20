#pragma once

#include "config/types.h"
#include "esp_err.h"

namespace config {

class ConfigManager {
public:
    ConfigManager() = default;

    /** Инициализация NVS и загрузка конфига. Вызывать один раз при старте. */
    esp_err_t init();

    /** Загрузить конфиг из NVS. */
    esp_err_t load();

    /** Сохранить конфиг в NVS. */
    esp_err_t save();

    /** Получить текущий конфиг (read-only). */
    const StationConfig &get() const { return cfg_; }

    /** Получить изменяемый конфиг (после изменения вызвать save()). */
    StationConfig &getMutable() { return cfg_; }

    /** Сохранить только timestamp в NVS (быстро, без перезаписи всего конфига). */
    esp_err_t saveTimestamp(uint32_t ts);

    /** Сбросить конфиг до значений по умолчанию. */
    void reset();

private:
    StationConfig cfg_;
    static constexpr const char *NVS_NS = "foxass";
};

} // namespace config
