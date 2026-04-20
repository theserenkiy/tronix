idf_component_register(
    SRCS "main.cpp"
    INCLUDE_DIRS "."
    REQUIRES
        "lora"
        "config"
        "driver"
        "esp_driver_spi"
        "esp_driver_gpio"
        "esp_timer"
        "freertos"
)
