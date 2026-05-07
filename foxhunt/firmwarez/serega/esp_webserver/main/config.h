// Константы для WiFi точки доступа
#define WIFI_SSID "LISA_1"        // Имя WiFi сети
#define WIFI_PASS "12345678"        // Пароль (минимум 8 символов)
#define WIFI_MAX_CONN 4             // Максимальное количество клиентов
#define DOMAIN_NAME "lisa"			// Full domain will be <DOMAIN_NAME>.local

// Константы для HTTP сервера
#define HTTP_PORT 80                // Порт HTTP сервера

static const char *TAG = "ESP32_AP_HTTP";

#define MOUNT_POINT "/files"
#define STATIC_URL_PATH "/static"  // URI путь для статики
#define STATIC_DIR "/static"  // Физический путь относительно MOUNT_POINT