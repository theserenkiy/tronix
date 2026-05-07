#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_http_server.h"
#include "esp_spiffs.h"
#include "mdns.h"

// Константы
#define WIFI_SSID "ESP32_AP"
#define WIFI_PASS "12345678"
#define WIFI_MAX_CONN 4
#define HTTP_PORT 80

// Параметры SPIFFS
#define MOUNT_POINT "/spiffs"
#define STATIC_PATH "/static"  // URI путь для статики
#define STATIC_DIR "/spiffs/static"  // Физический путь в SPIFFS

static const char *TAG = "ESP32_SERVER";

// MIME типы для файлов
static const char *get_mime_type(const char *filename)
{
    const char *ext = strrchr(filename, '.');
    if (!ext) return "application/octet-stream";
    
    if (strcmp(ext, ".html") == 0) return "text/html";
    if (strcmp(ext, ".css") == 0) return "text/css";
    if (strcmp(ext, ".js") == 0) return "application/javascript";
    if (strcmp(ext, ".json") == 0) return "application/json";
    if (strcmp(ext, ".png") == 0) return "image/png";
    if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) return "image/jpeg";
    if (strcmp(ext, ".gif") == 0) return "image/gif";
    if (strcmp(ext, ".ico") == 0) return "image/x-icon";
    if (strcmp(ext, ".svg") == 0) return "image/svg+xml";
    if (strcmp(ext, ".txt") == 0) return "text/plain";
    if (strcmp(ext, ".pdf") == 0) return "application/pdf";
    
    return "application/octet-stream";
}

// Обработчик статических файлов
static esp_err_t static_file_handler(httpd_req_t *req)
{
    char filepath[256];
    char full_path[256];
    
    // Получаем запрошенный URI (например: /static/style.css)
    const char *uri = req->uri;
    
    // Формируем полный путь в SPIFFS
    snprintf(filepath, sizeof(filepath), "%s%s", STATIC_DIR, uri + strlen(STATIC_PATH));
    
    ESP_LOGI(TAG, "Requesting file: %s", filepath);
    
    // Открываем файл
    FILE *file = fopen(filepath, "r");
    if (!file) {
        ESP_LOGW(TAG, "File not found: %s", filepath);
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }
    
    // Определяем MIME тип
    const char *mime = get_mime_type(filepath);
    httpd_resp_set_type(req, mime);
    
    // Читаем и отправляем файл по частям
    char buffer[1024];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        if (httpd_resp_send_chunk(req, buffer, bytes_read) != ESP_OK) {
            fclose(file);
            httpd_resp_sendstr_chunk(req, NULL);
            return ESP_FAIL;
        }
    }
    
    fclose(file);
    httpd_resp_send_chunk(req, NULL, 0);
    
    ESP_LOGI(TAG, "File sent: %s", filepath);
    return ESP_OK;
}

// Обработчик главной страницы
static esp_err_t root_get_handler(httpd_req_t *req)
{
    // Отправляем HTML файл из SPIFFS
    FILE *file = fopen("/spiffs/static/index.html", "r");
    if (!file) {
        const char *fallback = "<html><body><h1>ESP32 Server</h1><p>index.html not found</p></body></html>";
        httpd_resp_send(req, fallback, strlen(fallback));
        return ESP_OK;
    }
    
    httpd_resp_set_type(req, "text/html");
    char buffer[1024];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        httpd_resp_send_chunk(req, buffer, bytes_read);
    }
    httpd_resp_send_chunk(req, NULL, 0);
    fclose(file);
    
    return ESP_OK;
}

// Обработчик API (пример с параметрами)
static esp_err_t api_get_handler(httpd_req_t *req)
{
    char param_value[64];
    char resp_str[256];
    
    size_t query_len = httpd_req_get_url_query_len(req);
    if (query_len > 0) {
        char *query = malloc(query_len + 1);
        if (httpd_req_get_url_query_str(req, query, query_len + 1) == ESP_OK) {
            if (httpd_query_key_value(query, "name", param_value, sizeof(param_value)) == ESP_OK) {
                snprintf(resp_str, sizeof(resp_str), "{\"message\": \"Hello, %s!\"}", param_value);
            } else {
                snprintf(resp_str, sizeof(resp_str), "{\"error\": \"Parameter 'name' not found\"}");
            }
        }
        free(query);
    } else {
        snprintf(resp_str, sizeof(resp_str), "{\"message\": \"Use ?name=YourName\"}");
    }
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp_str, strlen(resp_str));
    return ESP_OK;
}



// Инициализация SPIFFS
static void init_spiffs(void)
{
    ESP_LOGI(TAG, "Initializing SPIFFS");
    
    esp_vfs_spiffs_conf_t conf = {
        .base_path = MOUNT_POINT,
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true
    };
    
    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return;
    }
    
    size_t total = 0, used = 0;
    ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition info (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }
}

// Инициализация WiFi AP
static void wifi_init_softap(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = WIFI_SSID,
            .ssid_len = strlen(WIFI_SSID),
            .password = WIFI_PASS,
            .max_connection = WIFI_MAX_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
        },
    };
    
    if (strlen(WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    ESP_LOGI(TAG, "WiFi AP started: %s", WIFI_SSID);
}

// Запуск HTTP сервера
static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;
    config.server_port = HTTP_PORT;
    
    if (httpd_start(&server, &config) == ESP_OK) {
        register_uri_handlers(server);
        ESP_LOGI(TAG, "HTTP server started on port %d", HTTP_PORT);
        return server;
    }
    
    ESP_LOGE(TAG, "Failed to start HTTP server");
    return NULL;
}

// Инициализация mDNS
static void init_mdns(void)
{
    ESP_ERROR_CHECK(mdns_init());
    ESP_ERROR_CHECK(mdns_hostname_set("esp32"));
    ESP_ERROR_CHECK(mdns_instance_name_set("ESP32 Web Server"));
    ESP_ERROR_CHECK(mdns_service_add(NULL, "_http", "_tcp", HTTP_PORT, NULL, 0));
    ESP_LOGI(TAG, "mDNS initialized: http://esp32.local/");
}

void app_main(void)
{
    // Инициализация NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // Инициализация SPIFFS
    init_spiffs();
    
    // Инициализация WiFi
    wifi_init_softap();
    
    // Инициализация mDNS
    init_mdns();
    
    // Запуск HTTP сервера
    httpd_handle_t server = start_webserver();
    if (!server) {
        ESP_LOGE(TAG, "Failed to start HTTP server");
        return;
    }
    
    ESP_LOGI(TAG, "==========================================");
    ESP_LOGI(TAG, "Server is running!");
    ESP_LOGI(TAG, "Connect to WiFi: %s", WIFI_SSID);
    ESP_LOGI(TAG, "Open browser: http://192.168.4.1/ or http://esp32.local/");
    ESP_LOGI(TAG, "Static files: http://192.168.4.1/static/style.css");
    ESP_LOGI(TAG, "API example: http://192.168.4.1/api?name=World");
    ESP_LOGI(TAG, "==========================================");
    
    // Основной цикл
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}