#pragma once

#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "host/ble_gatt.h" 
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "esp_bt.h"         // Содержит esp_ble_tx_power_set()
// #include "esp_private/esp_bluetooth_internal.h"

void ble_send(void *buf, int len);

static int ble_gatt_write_cb(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);

void ble_host_task(void *param);

void ble_app_advertise(void);

static void ble_app_on_sync(void);

void ble_init();
