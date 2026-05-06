#include "config/wifi_portal.h"

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_http_server.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include <cstring>
#include <cstdio>
#include <cinttypes>
#include <sys/time.h>

static const char *TAG = "WiFiPortal";

// ——— Минимальная HTML страница —————————————————————————————————————————————
static const char INDEX_HTML[] =
    "<!DOCTYPE html><html><head><meta charset='utf-8'>"
    "<title>FoxAss CP Config</title></head><body>"
    "<h1>FoxAss Checkpoint Config</h1>"
    "<p>API: <code>GET/POST /api/config</code>, "
    "<code>POST /api/sync_time</code>, "
    "<code>POST /api/start</code></p>"
    "<p>Use curl or the web app to configure.</p>"
    "</body></html>";

namespace config {

// Глобальный указатель для доступа из static handler'ов (одна инстанция)
static WiFiPortal *g_portal = nullptr;

WiFiPortal::WiFiPortal(ConfigManager &cfg_mgr) : cfg_mgr_(cfg_mgr)
{
    g_portal = this;
}

WiFiPortal::~WiFiPortal()
{
    stop();
    g_portal = nullptr;
}

esp_err_t WiFiPortal::start()
{
    if (running_) return ESP_OK;

    client_ever_connected_ = false;
    timed_out_             = false;

    snprintf(ap_ssid_, sizeof(ap_ssid_), "FoxAss-CP-%02X",
             cfg_mgr_.get().cp_num);

    // Если валидный конфиг есть — ждём 30 секунд;
    // если нет — ждём бесконечно пока не настроят.
    const uint32_t timeout_sec = cfg_mgr_.get().isValid() ? 30 : 0;

    ESP_LOGI(TAG, "Starting WiFi portal, SSID: %s, timeout: %s",
             ap_ssid_, timeout_sec ? "30s" : "infinite (no valid config)");

    esp_err_t ret = startWifi();
    if (ret != ESP_OK) return ret;

    ret = startHttpd();
    if (ret != ESP_OK) {
        stopWifi();
        return ret;
    }

    if (timeout_sec > 0) {
        esp_timer_create_args_t timer_args = {
            .callback        = &WiFiPortal::timeoutCallback,
            .arg             = this,
            .dispatch_method = ESP_TIMER_TASK,
            .name            = "wifi_portal_timeout",
            .skip_unhandled_events = false,
        };
        esp_timer_handle_t timer;
        esp_timer_create(&timer_args, &timer);
        esp_timer_start_once(timer, (uint64_t)timeout_sec * 1000000ULL);
        timer_handle_ = timer;
    }

    running_ = true;
    return ESP_OK;
}

esp_err_t WiFiPortal::stop()
{
    if (!running_) return ESP_OK;
    ESP_LOGI(TAG, "Stopping WiFi portal");

    if (timer_handle_) {
        esp_timer_stop(static_cast<esp_timer_handle_t>(timer_handle_));
        esp_timer_delete(static_cast<esp_timer_handle_t>(timer_handle_));
        timer_handle_ = nullptr;
    }

    stopHttpd();
    stopWifi();
    running_ = false;
    return ESP_OK;
}

void WiFiPortal::timeoutCallback(void *arg)
{
    WiFiPortal *self = static_cast<WiFiPortal *>(arg);
    ESP_LOGI(TAG, "WiFi portal timeout (no client connected), stopping");
    self->timed_out_ = true;
    self->stop();
    if (self->start_cb_) self->start_cb_();
}

void WiFiPortal::onClientConnected(void *arg, esp_event_base_t /*base*/,
                                    int32_t /*id*/, void * /*data*/)
{
    WiFiPortal *self = static_cast<WiFiPortal *>(arg);
    if (!self->client_ever_connected_) {
        self->client_ever_connected_ = true;
        ESP_LOGI(TAG, "Client connected to AP, cancelling auto-timeout");
        self->cancelTimeout();
    }
}

void WiFiPortal::cancelTimeout()
{
    if (timer_handle_) {
        esp_timer_stop(static_cast<esp_timer_handle_t>(timer_handle_));
        esp_timer_delete(static_cast<esp_timer_handle_t>(timer_handle_));
        timer_handle_ = nullptr;
        ESP_LOGI(TAG, "Auto-timeout cancelled");
    }
}

const char *WiFiPortal::stationType(uint8_t cp_num, bool is_fox)
{
    if (cp_num == 0x00)  return "start";
    if (cp_num == 0xFE)  return "finish";
    if (is_fox)          return "fox";
    return "regular";
}

// ——— WiFi ——————————————————————————————————————————————————————————————————

esp_err_t WiFiPortal::startWifi()
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));

    // Отменить таймаут при первом подключении клиента
    esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_STACONNECTED,
                                onClientConnected, this);

    wifi_config_t wifi_cfg = {};
    strncpy(reinterpret_cast<char *>(wifi_cfg.ap.ssid), ap_ssid_,
            sizeof(wifi_cfg.ap.ssid));
    wifi_cfg.ap.ssid_len      = strlen(ap_ssid_);
    wifi_cfg.ap.channel       = 6;
    wifi_cfg.ap.authmode      = WIFI_AUTH_OPEN;
    wifi_cfg.ap.max_connection = 2;

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_cfg));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi AP started: SSID=%s (open)", ap_ssid_);
    return ESP_OK;
}

void WiFiPortal::stopWifi()
{
    esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_AP_STACONNECTED,
                                  onClientConnected);
    esp_wifi_stop();
    esp_wifi_deinit();
}

// ——— HTTP server ———————————————————————————————————————————————————————————

esp_err_t WiFiPortal::startHttpd()
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = 8192;

    if (httpd_start(reinterpret_cast<httpd_handle_t *>(&httpd_handle_), &config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start httpd");
        return ESP_FAIL;
    }

    httpd_uri_t uris[] = {
        { .uri = "/",               .method = HTTP_GET,  .handler = handleIndex,     .user_ctx = nullptr },
        { .uri = "/api/config",     .method = HTTP_GET,  .handler = handleGetConfig, .user_ctx = nullptr },
        { .uri = "/api/config",     .method = HTTP_POST, .handler = handlePostConfig,.user_ctx = nullptr },
        { .uri = "/api/sync_time",  .method = HTTP_POST, .handler = handleSyncTime,  .user_ctx = nullptr },
        { .uri = "/api/start",      .method = HTTP_POST, .handler = handleStart,     .user_ctx = nullptr },
        { .uri = "/api/status",     .method = HTTP_GET,  .handler = handleStatus,    .user_ctx = nullptr },
    };

    for (auto &uri : uris) {
        httpd_register_uri_handler(static_cast<httpd_handle_t>(httpd_handle_), &uri);
    }

    ESP_LOGI(TAG, "HTTP server started");
    return ESP_OK;
}

void WiFiPortal::stopHttpd()
{
    if (httpd_handle_) {
        httpd_stop(static_cast<httpd_handle_t>(httpd_handle_));
        httpd_handle_ = nullptr;
    }
}

// ——— Вспомогательные утилиты ——————————————————————————————————————————————

static void send_json(httpd_req_t *req, const char *json, int status = 200)
{
    httpd_resp_set_type(req, "application/json");
    if (status != 200) {
        char code[8];
        snprintf(code, sizeof(code), "%d", status);
        httpd_resp_set_status(req, code);
    }
    httpd_resp_sendstr(req, json);
}

static int read_body(httpd_req_t *req, char *buf, size_t max_len)
{
    int total = req->content_len;
    if (total <= 0 || (size_t)total >= max_len) return -1;

    int received = 0;
    while (received < total) {
        int ret = httpd_req_recv(req, buf + received, total - received);
        if (ret <= 0) return -1;
        received += ret;
    }
    buf[received] = '\0';
    return received;
}

// Примитивный парсер JSON: ищет "key": value
static bool json_get_uint32(const char *json, const char *key, uint32_t *out)
{
    char search[64];
    snprintf(search, sizeof(search), "\"%s\":", key);
    const char *p = strstr(json, search);
    if (!p) return false;
    p += strlen(search);
    while (*p == ' ') p++;
    char *end;
    long long val = strtoll(p, &end, 10);
    if (end == p) return false;
    *out = (uint32_t)val;
    return true;
}

static bool json_get_str(const char *json, const char *key, char *out, size_t out_len)
{
    char search[64];
    snprintf(search, sizeof(search), "\"%s\":", key);
    const char *p = strstr(json, search);
    if (!p) return false;
    p += strlen(search);
    while (*p == ' ') p++;
    if (*p != '"') return false;
    p++;
    size_t i = 0;
    while (*p && *p != '"' && i + 1 < out_len) {
        out[i++] = *p++;
    }
    out[i] = '\0';
    return true;
}

static bool json_get_bool(const char *json, const char *key, bool *out)
{
    char search[64];
    snprintf(search, sizeof(search), "\"%s\":", key);
    const char *p = strstr(json, search);
    if (!p) return false;
    p += strlen(search);
    while (*p == ' ') p++;
    if (strncmp(p, "true", 4) == 0)  { *out = true;  return true; }
    if (strncmp(p, "false", 5) == 0) { *out = false; return true; }
    return false;
}

// Конвертирует hex-строку "AABBCC..." в байты
static bool hex_to_bytes(const char *hex, uint8_t *out, size_t len)
{
    if (strlen(hex) < len * 2) return false;
    for (size_t i = 0; i < len; i++) {
        char byte_str[3] = {hex[i*2], hex[i*2+1], '\0'};
        char *end;
        out[i] = (uint8_t)strtoul(byte_str, &end, 16);
        if (end != byte_str + 2) return false;
    }
    return true;
}

// ——— Handlers ——————————————————————————————————————————————————————————————

esp_err_t WiFiPortal::handleIndex(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_sendstr(req, INDEX_HTML);
    return ESP_OK;
}

esp_err_t WiFiPortal::handleGetConfig(httpd_req_t *req)
{
    if (!g_portal) { send_json(req, "{\"error\":\"no portal\"}", 500); return ESP_OK; }
    const StationConfig &cfg = g_portal->cfg_mgr_.get();

    char hex_station_key[33] = {};
    for (int i = 0; i < 16; i++) {
        snprintf(hex_station_key + i*2, 3, "%02x", cfg.station_key[i]);
    }
    char hex_sector_key[13] = {};
    for (int i = 0; i < 6; i++) {
        snprintf(hex_sector_key + i*2, 3, "%02x", cfg.sector_key[i]);
    }

    char json[640];
    snprintf(json, sizeof(json),
        "{"
        "\"cp_num\":%d,"
        "\"event_id\":%d,"
        "\"station_key\":\"%s\","
        "\"sector_key\":\"%s\","
        "\"station_type\":\"%s\","
        "\"is_fox\":%s,"
        "\"fox_on_sec\":%" PRIu32 ","
        "\"fox_off_sec\":%" PRIu32 ","
        "\"fox_morse\":\"%s\","
        "\"has_task\":%s,"
        "\"task_type\":%d,"
        "\"task_bonus\":%d,"
        "\"lora_freq_hz\":%" PRIu32 ","
        "\"fox_freq_hz\":%" PRIu32 ","
        "\"initialized\":%s"
        "}",
        cfg.cp_num, cfg.event_id,
        hex_station_key, hex_sector_key,
        stationType(cfg.cp_num, cfg.is_fox),
        cfg.is_fox ? "true" : "false",
        cfg.fox_on_sec, cfg.fox_off_sec, cfg.fox_morse,
        cfg.has_task ? "true" : "false",
        static_cast<int>(cfg.task.type), cfg.task.bonus_score,
        cfg.lora_freq_hz, cfg.fox_freq_hz,
        cfg.initialized ? "true" : "false"
    );

    send_json(req, json);
    return ESP_OK;
}

esp_err_t WiFiPortal::handlePostConfig(httpd_req_t *req)
{
    if (!g_portal) { send_json(req, "{\"error\":\"no portal\"}", 500); return ESP_OK; }

    char body[512];
    if (read_body(req, body, sizeof(body)) < 0) {
        send_json(req, "{\"error\":\"bad body\"}", 400);
        return ESP_OK;
    }

    StationConfig &cfg = g_portal->cfg_mgr_.getMutable();

    uint32_t u32;
    bool bval;
    char str[32];

    if (json_get_uint32(body, "cp_num",    &u32)) cfg.cp_num    = (uint8_t)u32;
    if (json_get_uint32(body, "event_id",  &u32)) cfg.event_id  = (uint16_t)u32;
    if (json_get_bool(body,   "is_fox",   &bval)) cfg.is_fox    = bval;
    if (json_get_uint32(body, "fox_on_sec",  &u32)) cfg.fox_on_sec  = u32;
    if (json_get_uint32(body, "fox_off_sec", &u32)) cfg.fox_off_sec = u32;
    if (json_get_str(body,    "fox_morse", str, sizeof(str)))
        strncpy(cfg.fox_morse, str, sizeof(cfg.fox_morse) - 1);
    if (json_get_bool(body,   "has_task",  &bval)) cfg.has_task  = bval;
    if (json_get_uint32(body, "task_type", &u32))
        cfg.task.type = static_cast<TaskType>(u32);
    if (json_get_uint32(body, "task_bonus",&u32)) cfg.task.bonus_score = (uint16_t)u32;
    if (json_get_uint32(body, "lora_freq_hz", &u32)) cfg.lora_freq_hz = u32;
    if (json_get_uint32(body, "fox_freq_hz",  &u32)) cfg.fox_freq_hz  = u32;

    char str32[33];
    if (json_get_str(body, "station_key", str32, sizeof(str32))) {
        hex_to_bytes(str32, cfg.station_key, 16);
    }
    char str12[13];
    if (json_get_str(body, "sector_key", str12, sizeof(str12))) {
        hex_to_bytes(str12, cfg.sector_key, 6);
    }

    cfg.initialized = true;
    esp_err_t ret = g_portal->cfg_mgr_.save();
    if (ret == ESP_OK) {
        send_json(req, "{\"ok\":true}");
    } else {
        send_json(req, "{\"error\":\"nvs save failed\"}", 500);
    }
    return ESP_OK;
}

esp_err_t WiFiPortal::handleSyncTime(httpd_req_t *req)
{
    char body[64];
    if (read_body(req, body, sizeof(body)) < 0) {
        send_json(req, "{\"error\":\"bad body\"}", 400);
        return ESP_OK;
    }

    uint32_t ts;
    if (!json_get_uint32(body, "timestamp", &ts)) {
        send_json(req, "{\"error\":\"missing timestamp\"}", 400);
        return ESP_OK;
    }

    struct timeval tv = { .tv_sec = (time_t)ts, .tv_usec = 0 };
    settimeofday(&tv, nullptr);

    // Сохраняем timestamp в NVS сразу — используется при ребуте без WiFi
    if (g_portal) {
        g_portal->cfg_mgr_.saveTimestamp(ts);
    }

    ESP_LOGI(TAG, "Time synchronized: %lu", (unsigned long)ts);
    send_json(req, "{\"ok\":true}");
    return ESP_OK;
}

esp_err_t WiFiPortal::handleStart(httpd_req_t *req)
{
    send_json(req, "{\"ok\":true,\"msg\":\"starting checkpoint\"}");

    if (g_portal) {
        // Отложенный старт: сначала отправляем ответ, потом останавливаем
        if (g_portal->start_cb_) g_portal->start_cb_();
        // stop() вызывается из callback'а (CheckpointLogic)
    }
    return ESP_OK;
}

esp_err_t WiFiPortal::handleStatus(httpd_req_t *req)
{
    if (!g_portal) { send_json(req, "{\"error\":\"no portal\"}", 500); return ESP_OK; }
    const StationConfig &cfg = g_portal->cfg_mgr_.get();

    uint32_t uptime_sec = (uint32_t)(esp_timer_get_time() / 1000000ULL);

    char json[256];
    snprintf(json, sizeof(json),
        "{"
        "\"portal_running\":true,"
        "\"config_valid\":%s,"
        "\"cp_num\":%d,"
        "\"station_type\":\"%s\","
        "\"client_connected\":%s,"
        "\"uptime_sec\":%" PRIu32
        "}",
        cfg.isValid() ? "true" : "false",
        cfg.cp_num,
        stationType(cfg.cp_num, cfg.is_fox),
        g_portal->client_ever_connected_ ? "true" : "false",
        uptime_sec);

    send_json(req, json);
    return ESP_OK;
}

} // namespace config
