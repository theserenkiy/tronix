idf_component_register(SRCS "main.c" "ble.c" "ble_net.c" "ble_commands.c" "led.c"
                    PRIV_REQUIRES nvs_flash bt esp_driver_gpio
                    INCLUDE_DIRS ".")
