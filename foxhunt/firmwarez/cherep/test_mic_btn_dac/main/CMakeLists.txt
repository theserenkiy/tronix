idf_component_register(
    SRCS "main.cpp"
    INCLUDE_DIRS "."
    REQUIRES
        "sensors"
        "led"
        "driver"
        "esp_driver_gpio"
        "esp_driver_dac"
        "esp_adc"
        "esp_timer"
        "freertos"
        "log"
)
