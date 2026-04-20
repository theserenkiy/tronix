idf_component_register(
    SRCS "src/player.cpp"
    INCLUDE_DIRS "include"
    REQUIRES
        "driver"
        "esp_driver_gpio"
        "esp_driver_ledc"
        "esp_driver_dac"
        "freertos"
        "esp_timer"
)
