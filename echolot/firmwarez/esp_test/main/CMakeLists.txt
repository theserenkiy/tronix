idf_component_register(SRCS "main.c" "lib.c" "sonar.c" "lcd.c" "sd.c" "gps.c" "cons.c" "uart_logger.c" "wav.c" "gen.c" "recorder.c" "encoder.c" "dsp.c" "ui.c" "views.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp_driver_mcpwm esp_driver_gpio esp_adc esp_driver_uart esp_driver_spi esp_driver_pcnt fatfs sdmmc st7735_driver
                    )
