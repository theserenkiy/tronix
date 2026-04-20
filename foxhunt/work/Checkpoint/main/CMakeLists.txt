idf_component_register(
    SRCS "main.cpp"
    INCLUDE_DIRS "."
    REQUIRES
        "checkpoint"
        "config"
        "driver"
        "nvs_flash"
        "esp_wifi"
        "esp_netif"
)
