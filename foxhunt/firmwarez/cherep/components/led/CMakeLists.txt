idf_component_register(
    SRCS "src/status_led.cpp"
    INCLUDE_DIRS "include"
    REQUIRES
        "driver"
        "esp_driver_gpio"
        "esp_driver_rmt"
        "led_strip"
        "esp_timer"
        "freertos"
)
