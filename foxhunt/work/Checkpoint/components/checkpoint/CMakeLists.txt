idf_component_register(
    SRCS
        "src/logic.cpp"
        "src/logger.cpp"
    INCLUDE_DIRS "include"
    REQUIRES
        "rfid"
        "lora"
        "sensors"
        "audio"
        "led"
        "config"
        "crypto"
        "driver"
        "esp_timer"
        "freertos"
)
