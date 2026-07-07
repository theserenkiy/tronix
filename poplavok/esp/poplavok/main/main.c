#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "nvs_flash.h"

// Корректный порядок подключения NimBLE для ESP-IDF 6.0
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "host/ble_gatt.h" 
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"


static const char *TAG = "BLE_ESP32";
uint8_t ble_addr_type;

// UUID службы и характеристики (произвольные 128-битные UUID)
// Убедитесь, что они совпадают с UUID в приложении Cordova!
// #define SERVICE_UUID      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF
// #define CHAR_UUID         0x01, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF

// Запись в формате Little-Endian (байты развернуты справо налево)
#define SERVICE_UUID      0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA, 0x99, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x00
#define CHAR_UUID         0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA, 0x99, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x01


static int ble_gatt_write_cb(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);

// Определение структуры GATT-сервера
static const struct ble_gatt_svc_def gatt_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID128_DECLARE(SERVICE_UUID),
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                .uuid = BLE_UUID128_DECLARE(CHAR_UUID),
                .access_cb = ble_gatt_write_cb, // Функция обработки чтения/записи
                .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
            },
            {0} // Конец характеристик
        },
    },
    {0} // Конец служб
};

// Колбэк, срабатывающий при получении данных от смартфона
static int ble_gatt_write_cb(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg) {
    if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
        char rx_buf[128] = {0};
        size_t len = OS_MBUF_PKTLEN(ctxt->om);
        
        if (len < sizeof(rx_buf)) {
            ble_hs_mbuf_to_flat(ctxt->om, rx_buf, len, NULL);
            ESP_LOGI(TAG, "Получены данные: %s", rx_buf);
            
            // Здесь можно обработать входящую строку
        }
        return 0;
    }
    return BLE_ATT_ERR_UNLIKELY;
}

// Запуск рекламы (Advertising) устройства в эфир
void ble_app_advertise(void) {
    struct ble_gap_adv_params adv_params;
    struct ble_hs_adv_fields fields;
    int rc;

    memset(&fields, 0, sizeof(fields));
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    
    // Имя устройства в эфире
    const char *name = "ESP32_GATT_SERVER";
    fields.name = (uint8_t *)name;
    fields.name_len = strlen(name);
    fields.name_is_complete = 1;

    rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) {
        ESP_LOGE(TAG, "Ошибка настройки полей рекламы: %d", rc);
        return;
    }

    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

    rc = ble_gap_adv_start(ble_addr_type, NULL, BLE_HS_FOREVER, &adv_params, NULL, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "Ошибка запуска рекламы: %d", rc);
    }
}

static void ble_app_on_sync(void) {
    ble_hs_id_infer_auto(0, &ble_addr_type);
    ble_app_advertise();
}

void ble_host_task(void *param) {
    nimble_port_run();
    nimble_port_freertos_deinit();
}

void app_main(void) {
    // Инициализация NVS (требуется для BLE)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Инициализация NimBLE
    ESP_ERROR_CHECK(nimble_port_init());
    
    ble_hs_cfg.sync_cb = ble_app_on_sync;
    
    int rc = ble_gatts_count_cfg(gatt_svcs);
    if (rc != 0) return;
    
    rc = ble_gatts_add_svcs(gatt_svcs);
    if (rc != 0) return;

    nimble_port_freertos_init(ble_host_task);
}
