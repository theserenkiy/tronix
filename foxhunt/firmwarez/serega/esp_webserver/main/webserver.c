#include <stdio.h>
#include <string.h>
#include "esp_event.h"
#include "esp_log.h"
#include "config.h"
#include "webserver.h"


// Обработчик корневого URL ("/")
static esp_err_t root_get_handler(httpd_req_t *req)
{
    const char* resp_str = (const char*) req->user_ctx;
    send_file("/static/index.html")
    return ESP_OK;
}


// Обработчик с поддержкой query-параметров
static esp_err_t api_get_handler(httpd_req_t *req)
{
    char param_value[64];
    char resp_str[512];
    
    // Получаем значение параметра "name"
    size_t param_len = httpd_req_get_url_query_len(req) + 1;
    if (param_len > 1) {
        char* query = malloc(param_len);
        if (httpd_req_get_url_query_str(req, query, param_len) == ESP_OK) {
            // Парсим конкретный параметр
            if (httpd_query_key_value(query, "name", param_value, sizeof(param_value)) == ESP_OK) {
                snprintf(resp_str, sizeof(resp_str), 
                        "{\"message\": \"Hello, %s!\"}", param_value);
            } 
            else if (httpd_query_key_value(query, "id", param_value, sizeof(param_value)) == ESP_OK) {
                snprintf(resp_str, sizeof(resp_str), 
                        "{\"id\": %s, \"status\": \"ok\"}", param_value);
            }
            else {
                snprintf(resp_str, sizeof(resp_str), 
                        "{\"error\": \"Parameter 'name' or 'id' not found\"}");
            }
        }
        free(query);
    } else {
        snprintf(resp_str, sizeof(resp_str), 
                "{\"message\": \"No parameters provided. Use ?name=YourName\"}");
    }
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp_str, strlen(resp_str));
    return ESP_OK;
}

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


static esp_err_t send_file(char* filepath)
{
    snprintf(filepath, sizeof(filepath), "%s%s", MOUNT_POINT, filepath);

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


// Обработчик статических файлов
static esp_err_t static_file_handler(httpd_req_t *req)
{
    char filepath[256];
    char full_path[256];
    
    // Получаем запрошенный URI (например: /static/style.css)
    const char *uri = req->uri;
    
    // Формируем полный путь
    snprintf(filepath, sizeof(filepath), "%s%s", STATIC_DIR, uri + strlen(STATIC_URL_PATH));
    
    return send_file(filepath);
}

// Регистрация URI обработчиков
static void register_uri_handlers(httpd_handle_t server)
{
    // Статические файлы (должен быть первым, чтобы перехватывать все /static/*)
    httpd_uri_t static_uri = {
        .uri       = STATIC_PATH,
        .method    = HTTP_GET,
        .handler   = static_file_handler,
        .user_ctx  = NULL,
        .is_websocket = false,
        .handle_ws_control_frames = false,
        .supported_subprotocol = NULL
    };
    // Для обработки всех файлов в /static нужно использовать URI с wildcard
    static_uri.uri = "/static/*";
    httpd_register_uri_handler(server, &static_uri);
    
    // Другие обработчики
    httpd_uri_t root_uri = {
        .uri       = "/",
        .method    = HTTP_GET,
        .handler   = root_get_handler,
        .user_ctx  = NULL
    };
    httpd_register_uri_handler(server, &root_uri);
    
    httpd_uri_t api_uri = {
        .uri       = "/api",
        .method    = HTTP_GET,
        .handler   = api_get_handler,
        .user_ctx  = NULL
    };
    httpd_register_uri_handler(server, &api_uri);
}

// Запуск HTTP сервера
httpd_handle_t webserver_start(int port)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;
    config.server_port = port;
    
    // Запускаем HTTP сервер
    if (httpd_start(&server, &config) == ESP_OK) {
        // Регистрируем URI обработчики
        register_uri_handlers(server);
        ESP_LOGI(TAG, "HTTP server started on port %d", HTTP_PORT);
        return server;
    }
    
    ESP_LOGE(TAG, "Failed to start HTTP server");
    return NULL;
}