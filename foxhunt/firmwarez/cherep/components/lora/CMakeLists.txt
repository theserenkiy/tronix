idf_component_register(
    SRCS
        "src/sx1276.cpp"
        "src/protocol.cpp"
        "src/fox_mode.cpp"
    INCLUDE_DIRS "include"
    REQUIRES
        "driver"
        "esp_driver_spi"
        "esp_driver_gpio"
        "config"
        "esp_timer"
        "esp_rom"
        "freertos"
)
