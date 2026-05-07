#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "config.h"
#include "webserver.h"
#include "mdns.h"
#include "esp_littlefs.h"



static void init_mdns(void)
{
    // Инициализация mDNS
    ESP_ERROR_CHECK(mdns_init());
    
    // Устанавливаем hostname
    ESP_ERROR_CHECK(mdns_hostname_set(DOMAIN_NAME));
    ESP_ERROR_CHECK(mdns_instance_name_set("ESP32 Access Point"));
    
    // Добавляние сервиса HTTP
    ESP_ERROR_CHECK(mdns_service_add(NULL, "_http", "_tcp", 80, NULL, 0));
    
    ESP_LOGI(TAG, "mDNS initialized");
    ESP_LOGI(TAG, "You can access: http://%s.local/", DOMAIN_NAME);
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
            .authmode = strlen(WIFI_PASS) > 0 ? WIFI_AUTH_WPA_WPA2_PSK : WIFI_AUTH_OPEN,
        },
    };
    
    // Проверка длины пароля
    if (strlen(WIFI_PASS) > 0 && strlen(WIFI_PASS) < 8) {
        ESP_LOGW(TAG, "Password is too short! Using open network");
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    // Выводим информацию о точке доступа
    ESP_LOGI(TAG, "WiFi AP started");
    ESP_LOGI(TAG, "SSID: %s", WIFI_SSID);
    if (strlen(WIFI_PASS) >= 8) {
        ESP_LOGI(TAG, "Password: %s", WIFI_PASS);
    } else {
        ESP_LOGI(TAG, "No password (open network)");
    }
    // ESP_LOGI(TAG, "IP Address: " IPSTR, IP2STR(&wifi_config.ap.ip));
}

static void init_littlefs(void)
{
    ESP_LOGI(TAG, "Initializing LittleFS");
    
    esp_vfs_littlefs_conf_t conf = {
        .base_path = MOUNT_POINT,
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true
    };
    
    esp_err_t ret = esp_vfs_littlefs_register(&conf);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find LittleFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize LittleFS (%s)", esp_err_to_name(ret));
        }
        return;
    }
    
    size_t total = 0, used = 0;
    ret = esp_littlefs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get LittleFS partition info (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }
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
    
    init_littlefs();

    // Инициализация WiFi точки доступа
    wifi_init_softap();

	init_mdns();
    
    // Задержка для стабилизации WiFi
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    // Запуск HTTP сервера
    if (webserver_start(HTTP_PORT) == NULL) {
        ESP_LOGE(TAG, "Failed to start HTTP server");
        return;
    }
    
    ESP_LOGI(TAG, "System ready! Connect to WiFi '%s' password: '%s' and open http://%s.local/", WIFI_SSID, WIFI_PASS, DOMAIN_NAME);
    ESP_LOGI(TAG, "Available endpoints:");
    ESP_LOGI(TAG, "  - /");
    ESP_LOGI(TAG, "  - /hello");
    ESP_LOGI(TAG, "  - /status");
    
    // Основной цикл - ничего не делаем, просто ждем
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}	