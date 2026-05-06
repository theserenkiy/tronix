#pragma once

#include "config/manager.h"
#include "esp_err.h"
#include "esp_http_server.h"
#include <functional>

namespace config {

/**
 * WiFi AP + HTTP-сервер конфигурации.
 *
 * При старте поднимает точку доступа "FoxAss-CP-XX" на 10 минут.
 * Предоставляет JSON REST API для настройки станции.
 * Автоматически отключается по таймауту или вызову stop().
 */
class WiFiPortal {
public:
    /** Callback вызывается когда получена команда /api/start */
    using StartCallback = std::function<void()>;

    explicit WiFiPortal(ConfigManager &cfg_mgr);
    ~WiFiPortal();

    /**
     * Запустить WiFi AP и HTTP-сервер.
     * Таймаут определяется автоматически:
     *   - нет валидного конфига → ждём бесконечно
     *   - есть валидный конфиг  → 30 секунд; при подключении клиента — бесконечно
     */
    esp_err_t start();

    /** Остановить WiFi и HTTP-сервер. */
    esp_err_t stop();

    /** Установить callback для команды /api/start */
    void setStartCallback(StartCallback cb) { start_cb_ = cb; }

    bool isRunning() const { return running_; }

    /**
     * Вернуть true если портал завершился по таймауту (клиент не подключался).
     * В этом случае нужно восстановить время из NVS.
     */
    bool timedOut() const { return timed_out_; }

private:
    static esp_err_t handleGetConfig(httpd_req_t *req);
    static esp_err_t handlePostConfig(httpd_req_t *req);
    static esp_err_t handleSyncTime(httpd_req_t *req);
    static esp_err_t handleStart(httpd_req_t *req);
    static esp_err_t handleStatus(httpd_req_t *req);
    static esp_err_t handleIndex(httpd_req_t *req);

    static void timeoutCallback(void *arg);
    static void onClientConnected(void *arg, esp_event_base_t base,
                                  int32_t id, void *data);

    /** "start" | "finish" | "fox" | "regular" */
    static const char *stationType(uint8_t cp_num, bool is_fox);

    esp_err_t startWifi();
    esp_err_t startHttpd();
    void stopWifi();
    void stopHttpd();
    void cancelTimeout();

    ConfigManager &cfg_mgr_;
    StartCallback  start_cb_;
    void          *httpd_handle_    = nullptr;
    void          *timer_handle_    = nullptr;
    bool           running_         = false;
    bool           client_ever_connected_ = false;
    bool           timed_out_       = false;
    char           ap_ssid_[32]     = {};
};

} // namespace config
