idf_component_register(
    SRCS "src/st7735.c" 
         "src/graphics.c"
    INCLUDE_DIRS "include"
    REQUIRES driver esp_timer log esp_hw_support esp_driver_spi esp_driver_gpio
    PRIV_REQUIRES main
)