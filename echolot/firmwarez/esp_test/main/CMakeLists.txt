idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    # REQUIRES esp_driver_ledc esp_driver_gptimer
                    REQUIRES esp_driver_mcpwm esp_driver_gpio
                    )
