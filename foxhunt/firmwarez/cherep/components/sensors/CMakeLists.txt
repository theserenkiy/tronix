idf_component_register(
    SRCS
        "src/mq3.cpp"
        "src/button.cpp"
    INCLUDE_DIRS "include"
    REQUIRES
        "driver"
        "esp_driver_gpio"
        "esp_adc"
        "esp_timer"
        "freertos"
)
