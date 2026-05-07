idf_component_register(SRCS "main.c" "webserver.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp_wifi mdns nvs_flash esp_http_server littlefs)

set(PARTITION_TABLE "../partitions.csv")
littlefs_create_partition_image(storage ../filesystem)  # FLASH_IN_PROJECT)