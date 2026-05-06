idf_component_register(
    SRCS
        "src/rc522.cpp"
        "src/mifare.cpp"
        "src/card_processor.cpp"
    INCLUDE_DIRS "include"
    REQUIRES
        "driver"
        "esp_driver_spi"
        "esp_driver_gpio"
        "crypto"
        "esp_timer"
)
