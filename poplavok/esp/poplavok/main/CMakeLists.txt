idf_component_register(SRCS "main.c" "ble.c" "ble_net_layer.c" "ble_commands.c"
                    PRIV_REQUIRES nvs_flash bt
                    INCLUDE_DIRS ".")
