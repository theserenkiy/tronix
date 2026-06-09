#include "sonar_adc.h"

#include <string.h>

#include "driver/uart.h"
#include "esp_log.h"

#include "esp_adc/adc_continuous.h"
#include "hal/adc_types.h"



static adc_continuous_handle_t adc_handle = NULL;

esp_err_t sonar_adc_init(void)
{
    esp_log_level_set("*", ESP_LOG_NONE);

    uart_set_baudrate(UART_PORT, UART_BAUDRATE);

    adc_continuous_handle_cfg_t adc_config = {
        .max_store_buf_size = 32768,
        .conv_frame_size = 1024,
    };

    ESP_ERROR_CHECK(
        adc_continuous_new_handle(&adc_config, &adc_handle)
    );

    adc_digi_pattern_config_t pattern = {
        .atten     = ADC_ATTEN_DB_0,
        .channel   = ADC_CHANNEL_USED,
        .unit      = ADC_UNIT_USED,
        .bit_width = ADC_BITWIDTH_12,
    };

    adc_continuous_config_t dig_cfg = {
        .sample_freq_hz = ADC_SAMPLE_FREQ_HZ,
        .conv_mode      = ADC_CONV_SINGLE_UNIT_1,
        .format         = ADC_DIGI_OUTPUT_FORMAT_TYPE1,
        .pattern_num    = 1,
        .adc_pattern    = &pattern,
    };

    ESP_ERROR_CHECK(
        adc_continuous_config(adc_handle, &dig_cfg)
    );

    return ESP_OK;
}


esp_err_t sonar_adc_capture(uint16_t *buffer, size_t samples)
{
    printf("ADC record started...\n");

    uint8_t dma_buf[1024];

    size_t collected = 0;

    ESP_ERROR_CHECK(
        adc_continuous_start(adc_handle)
    );

    while (collected < samples)
    {
        uint32_t bytes_read = 0;

        esp_err_t ret =
            adc_continuous_read(
                adc_handle,
                dma_buf,
                sizeof(dma_buf),
                &bytes_read,
                1000);

        if (ret != ESP_OK)
            continue;

        uint32_t results =
            bytes_read / sizeof(adc_digi_output_data_t);

        adc_digi_output_data_t *p =
            (adc_digi_output_data_t *)dma_buf;

        for (uint32_t i = 0;
             i < results && collected < samples;
             i++)
        {
            buffer[collected++] = p[i].type1.data;
        }
    }

    ESP_ERROR_CHECK(
        adc_continuous_stop(adc_handle)
    );

    printf("ADC record done\n");

    return ESP_OK;
}

esp_err_t sonar_uart_send_buffer(uint16_t *buffer,
                                 size_t samples)
{
    size_t bytes =
        samples * sizeof(uint16_t);

    int sent =
        uart_write_bytes(
            UART_PORT,
            (const char *)buffer,
            bytes);

    return (sent == bytes)
           ? ESP_OK
           : ESP_FAIL;
}


