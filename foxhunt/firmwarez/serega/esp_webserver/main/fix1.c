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

// Константы для WiFi точки доступа
#define WIFI_SSID "ESP32_AP"        // Имя WiFi сети
#define WIFI_PASS "12345678"        // Пароль (минимум 8 символов)
#define WIFI_MAX_CONN 4             // Максимальное количество клиентов

// Константы для HTTP сервера
#define HTTP_PORT 80                // Порт HTTP сервера

static const char *TAG = "ESP32_AP_HTTP";

// Функция для получения количества подключенных клиентов
static int get_ap_client_count(void)
{
    wifi_sta_list_t sta_list;
    memset(&sta_list, 0, sizeof(sta_list));
    
    if (esp_wifi_ap_get_sta_list(&sta_list) == ESP_OK) {
        return sta_list.num;
    }
    return 0;
}

// Обработчик корневого URL ("/")
static esp_err_t root_get_handler(httpd_req_t *req)
{
    const char* resp_str = (const char*) req->user_ctx;
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, resp_str, strlen(resp_str));
    return ESP_OK;
}

// Обработчик URL "/hello"
static esp_err_t hello_get_handler(httpd_req_t *req)
{
    const char* resp_str = "<html><body><h1>Hello from ESP32!</h1></body></html>";
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, resp_str, strlen(resp_str));
    return ESP_OK;
}

// Обработчик URL "/status"
static esp_err_t status_get_handler(httpd_req_t *req)
{
    char resp_str[512];
    
    // Получаем информацию о WiFi
    wifi_config_t ap_config;
    esp_wifi_get_config(WIFI_IF_AP, &ap_config);
    
    // Получаем количество подключенных клиентов
    int client_count = get_ap_client_count();
    
    // Формируем JSON ответ
    snprintf(resp_str, sizeof(resp_str),
        "{\n"
        "  \"status\": \"running\",\n"
        "  \"ssid\": \"%s\",\n"
        "  \"port\": %d,\n"
        "  \"clients\": %d\n"
        "}",
        ap_config.ap.ssid, HTTP_PORT, client_count);
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp_str, strlen(resp_str));
    return ESP_OK;
}

// URI конфигурация для корневого пути
static const httpd_uri_t root_uri = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = root_get_handler,
    .user_ctx  = "<html><body><h1>ESP32 Access Point</h1>"
                 "<p>Available endpoints:</p>"
                 "<ul>"
                 "<li><a href=\"/hello\">/hello</a></li>"
                 "<li><a href=\"/status\">/status</a></li>"
                 "</ul>"
                 "</body></html>"
};

// URI конфигурация для пути "/hello"
static const httpd_uri_t hello_uri = {
    .uri       = "/hello",
    .method    = HTTP_GET,
    .handler   = hello_get_handler,
    .user_ctx  = NULL
};

// URI конфигурация для пути "/status"
static const httpd_uri_t status_uri = {
    .uri       = "/status",
    .method    = HTTP_GET,
    .handler   = status_get_handler,
    .user_ctx  = NULL
};

// Запуск HTTP сервера
static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;
    config.server_port = HTTP_PORT;
    
    // Запускаем HTTP сервер
    if (httpd_start(&server, &config) == ESP_OK) {
        // Регистрируем URI обработчики
        httpd_register_uri_handler(server, &root_uri);
        httpd_register_uri_handler(server, &hello_uri);
        httpd_register_uri_handler(server, &status_uri);
        ESP_LOGI(TAG, "HTTP server started on port %d", HTTP_PORT);
        return server;
    }
    
    ESP_LOGE(TAG, "Failed to start HTTP server");
    return NULL;
}

// Инициализация WiFi в режиме точки доступа
static void wifi_init_softap(void)
{
    // Инициализация сетевого интерфейса
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    // Настройка точки доступа
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = WIFI_SSID,
            .ssid_len = strlen(WIFI_SSID),
            .password = WIFI_PASS,
            .max_connection = WIFI_MAX_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
        },
    };
    
    // Проверка длины пароля
    if (strlen(WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
        ESP_LOGI(TAG, "Open network (no password)");
    } else if (strlen(WIFI_PASS) < 8) {
        ESP_LOGW(TAG, "Password is too short (min 8 chars)! Setting open network");
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
        memset(wifi_config.ap.password, 0, sizeof(wifi_config.ap.password));
    }
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    // Выводим информацию о точке доступа
    ESP_LOGI(TAG, "==========================================");
    ESP_LOGI(TAG, "WiFi AP started");
    ESP_LOGI(TAG, "SSID: %s", WIFI_SSID);
    if (wifi_config.ap.authmode != WIFI_AUTH_OPEN) {
        ESP_LOGI(TAG, "Password: %s", WIFI_PASS);
    } else {
        ESP_LOGI(TAG, "Password: (open network)");
    }
    ESP_LOGI(TAG, "Max connections: %d", WIFI_MAX_CONN);
    ESP_LOGI(TAG, "==========================================");
}

void app_main(void)
{
    // Инициализация NVS (необходимо для WiFi)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // Инициализация WiFi точки доступа
    wifi_init_softap();
    
    // Задержка для стабилизации WiFi
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    // Запуск HTTP сервера
    httpd_handle_t server = start_webserver();
    
    if (server == NULL) {
        ESP_LOGE(TAG, "Failed to start HTTP server");
        return;
    }
    
    // Получаем IP адрес точки доступа
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
    esp_netif_ip_info_t ip_info;
    esp_netif_get_ip_info(netif, &ip_info);
    
    ESP_LOGI(TAG, "==========================================");
    ESP_LOGI(TAG, "HTTP Server is running!");
    ESP_LOGI(TAG, "Connect to WiFi '%s'", WIFI_SSID);
    ESP_LOGI(TAG, "Open browser and navigate to:");
    ESP_LOGI(TAG, "  http://" IPSTR "/", IP2STR(&ip_info.ip));
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Available endpoints:");
    ESP_LOGI(TAG, "  - http://" IPSTR "/", IP2STR(&ip_info.ip));
    ESP_LOGI(TAG, "  - http://" IPSTR "/hello", IP2STR(&ip_info.ip));
    ESP_LOGI(TAG, "  - http://" IPSTR "/status", IP2STR(&ip_info.ip));
    ESP_LOGI(TAG, "==========================================");
    
    // Основной цикл - ничего не делаем, просто ждем
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}