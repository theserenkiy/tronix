idf_component_register(
    SRCS "main.cpp"
    INCLUDE_DIRS "."
    REQUIRES "rfid" "driver" "esp_driver_spi" "esp_driver_gpio" "freertos"
)
