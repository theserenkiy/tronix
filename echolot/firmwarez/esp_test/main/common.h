#pragma once

#include "driver/gpio.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <inttypes.h>



#define TX_GPIO_1		13
#define TX_GPIO_2		14
#define IR_DSBL_PIN		26

#define TX_FREQ_HZ   187000
#define BURST_TO_BURST_DELAY_MS 40
#define IR_DSBL_DELAY_US 1000


#define ADC_UNIT_USED           ADC_UNIT_1
#define ADC_CHANNEL_USED        ADC_CHANNEL_6     // GPIO34
#define ADC_SAMPLE_FREQ_HZ      1000000

#define ADC_RECORD_TIME_MS		40

#define ADC_RECORD_SAMPLES		(int)(ADC_RECORD_TIME_MS*ADC_SAMPLE_FREQ_HZ/1000)

#define UART_PORT               UART_NUM_0
#define UART_BAUDRATE           921600