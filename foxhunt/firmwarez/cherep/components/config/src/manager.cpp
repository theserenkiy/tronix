#include "config/manager.h"

#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include <cstring>

static const char *TAG = "ConfigMgr";

namespace config {

esp_err_t ConfigManager::init()
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS flash erase needed");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "NVS init failed: %s", esp_err_to_name(ret));
        return ret;
    }
    return load();
}

esp_err_t ConfigManager::load()
{
    nvs_handle_t h;
    esp_err_t ret = nvs_open(NVS_NS, NVS_READONLY, &h);
    if (ret == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "No config in NVS, using defaults");
        reset();
        return ESP_OK;
    }
    if (ret != ESP_OK) return ret;

    uint8_t u8;
    uint16_t u16;
    uint32_t u32;
    size_t len;

    if (nvs_get_u8(h, "cp_num", &u8)    == ESP_OK) cfg_.cp_num    = u8;
    if (nvs_get_u16(h, "event_id", &u16) == ESP_OK) cfg_.event_id = u16;

    len = sizeof(cfg_.station_key);
    nvs_get_blob(h, "station_key", cfg_.station_key, &len);

    len = sizeof(cfg_.sector_key);
    nvs_get_blob(h, "sector_key", cfg_.sector_key, &len);

    uint8_t is_fox = 0;
    if (nvs_get_u8(h, "is_fox", &is_fox) == ESP_OK) cfg_.is_fox = (is_fox != 0);
    if (nvs_get_u32(h, "fox_on_sec",  &u32) == ESP_OK) cfg_.fox_on_sec  = u32;
    if (nvs_get_u32(h, "fox_off_sec", &u32) == ESP_OK) cfg_.fox_off_sec = u32;

    len = sizeof(cfg_.fox_morse);
    nvs_get_str(h, "fox_morse", cfg_.fox_morse, &len);

    uint8_t has_task = 0;
    if (nvs_get_u8(h, "has_task", &has_task) == ESP_OK) cfg_.has_task = (has_task != 0);
    if (nvs_get_u8(h, "task_type", &u8) == ESP_OK)
        cfg_.task.type = static_cast<TaskType>(u8);
    if (nvs_get_u16(h, "task_bonus", &u16) == ESP_OK) cfg_.task.bonus_score = u16;

    if (nvs_get_u32(h, "lora_freq", &u32) == ESP_OK) cfg_.lora_freq_hz = u32;
    if (nvs_get_u32(h, "fox_freq",  &u32) == ESP_OK) cfg_.fox_freq_hz  = u32;

    uint8_t init_flag = 0;
    if (nvs_get_u8(h, "initialized", &init_flag) == ESP_OK)
        cfg_.initialized = (init_flag != 0);

    if (nvs_get_u32(h, "timestamp", &u32) == ESP_OK) cfg_.last_timestamp = u32;

    nvs_close(h);
    ESP_LOGI(TAG, "Config loaded: cp_num=%d event_id=%d initialized=%d",
             cfg_.cp_num, cfg_.event_id, (int)cfg_.initialized);
    return ESP_OK;
}

esp_err_t ConfigManager::save()
{
    nvs_handle_t h;
    esp_err_t ret = nvs_open(NVS_NS, NVS_READWRITE, &h);
    if (ret != ESP_OK) return ret;

    nvs_set_u8(h,  "cp_num",      cfg_.cp_num);
    nvs_set_u16(h, "event_id",    cfg_.event_id);
    nvs_set_blob(h, "station_key", cfg_.station_key, sizeof(cfg_.station_key));
    nvs_set_blob(h, "sector_key",  cfg_.sector_key,  sizeof(cfg_.sector_key));
    nvs_set_u8(h,  "is_fox",      cfg_.is_fox ? 1 : 0);
    nvs_set_u32(h, "fox_on_sec",  cfg_.fox_on_sec);
    nvs_set_u32(h, "fox_off_sec", cfg_.fox_off_sec);
    nvs_set_str(h, "fox_morse",   cfg_.fox_morse);
    nvs_set_u8(h,  "has_task",    cfg_.has_task ? 1 : 0);
    nvs_set_u8(h,  "task_type",   static_cast<uint8_t>(cfg_.task.type));
    nvs_set_u16(h, "task_bonus",  cfg_.task.bonus_score);
    nvs_set_u32(h, "lora_freq",   cfg_.lora_freq_hz);
    nvs_set_u32(h, "fox_freq",    cfg_.fox_freq_hz);
    nvs_set_u8(h,  "initialized", cfg_.initialized ? 1 : 0);
    nvs_set_u32(h, "timestamp",   cfg_.last_timestamp);

    ret = nvs_commit(h);
    nvs_close(h);

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Config saved to NVS");
    }
    return ret;
}

esp_err_t ConfigManager::saveTimestamp(uint32_t ts)
{
    cfg_.last_timestamp = ts;
    nvs_handle_t h;
    esp_err_t ret = nvs_open(NVS_NS, NVS_READWRITE, &h);
    if (ret != ESP_OK) return ret;
    nvs_set_u32(h, "timestamp", ts);
    ret = nvs_commit(h);
    nvs_close(h);
    return ret;
}

void ConfigManager::reset()
{
    cfg_ = StationConfig{};
}

} // namespace config
