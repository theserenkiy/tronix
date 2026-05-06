idf_component_register(
    SRCS "main.cpp"
    INCLUDE_DIRS "."
    REQUIRES
        "config"
        "crypto"
        "esp_http_server"
        "esp_timer"
        "freertos"
        "driver"
        "esp_driver_gpio"
)
