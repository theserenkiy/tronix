idf_component_register(SRCS "main.c" "sonar_tx.c" "sonar_adc.c" "lcd.c"
                    INCLUDE_DIRS "."
                    # REQUIRES esp_driver_ledc esp_driver_gptimer
                    REQUIRES esp_driver_mcpwm esp_driver_gpio esp_adc esp_driver_uart esp_driver_spi
                    )
