idf_component_register(SRCS "main.c" "lib.c" "sonar.c" "lcd2.c" "sd.c" "gps.c" "cons.c" "uart_logger.c" "wav.c" "chirp.c" "recorder.c"
                    INCLUDE_DIRS "."
                    # REQUIRES esp_driver_ledc esp_driver_gptimer
                    REQUIRES esp_driver_mcpwm esp_driver_gpio esp_adc esp_driver_uart esp_driver_spi fatfs sdmmc st7735_driver
                    )
