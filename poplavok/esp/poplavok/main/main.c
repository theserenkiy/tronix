#include "common.h"

#include "nvs_flash.h"

#include "ble.h"
#include "led.h"



void app_main(void) {
	// Инициализация NVS (требуется для BLE)
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);

	ble_init();

	led_init();

	while (1)
	{
		vTaskDelay(pdMS_TO_TICKS(2000));
		// ble_send(str, strlen(str));
	}
	
}