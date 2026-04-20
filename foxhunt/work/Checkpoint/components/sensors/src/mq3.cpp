#include "sensors/mq3.h"
#include "esp_log.h"

static const char *TAG = "MQ3";

namespace sensors {

MQ3Sensor::MQ3Sensor(gpio_num_t heater_gpio, adc_channel_t adc_channel,
                     adc_unit_t adc_unit)
    : heater_gpio_(heater_gpio), adc_channel_(adc_channel), adc_unit_(adc_unit)
{}

esp_err_t MQ3Sensor::init()
{
    // Настроить heater GPIO
    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << heater_gpio_);
    io_conf.mode         = GPIO_MODE_OUTPUT;
    io_conf.pull_up_en   = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type    = GPIO_INTR_DISABLE;
    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) return ret;

    gpio_set_level(heater_gpio_, 0); // Heater off initially

    // ADC oneshot init
    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id  = adc_unit_,
        .clk_src  = ADC_RTC_CLK_SRC_DEFAULT,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ret = adc_oneshot_new_unit(&unit_cfg, &adc_handle_);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ADC unit init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten    = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    ret = adc_oneshot_config_channel(adc_handle_, adc_channel_, &chan_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ADC channel config failed: %s", esp_err_to_name(ret));
        return ret;
    }

    inited_ = true;
    ESP_LOGI(TAG, "MQ3 initialized (heater GPIO=%d)", heater_gpio_);
    return ESP_OK;
}

void MQ3Sensor::heatOn()
{
    gpio_set_level(heater_gpio_, 1);
    heating_ = true;
    ESP_LOGI(TAG, "Heater ON");
}

void MQ3Sensor::heatOff()
{
    gpio_set_level(heater_gpio_, 0);
    heating_ = false;
    ESP_LOGI(TAG, "Heater OFF");
}

uint16_t MQ3Sensor::readRaw()
{
    if (!inited_ || !adc_handle_) return 0;

    int raw = 0;
    esp_err_t ret = adc_oneshot_read(adc_handle_, adc_channel_, &raw);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "ADC read failed: %s", esp_err_to_name(ret));
        return 0;
    }
    return (uint16_t)raw;
}

void MQ3Sensor::deinit()
{
    heatOff();
    if (adc_handle_) {
        adc_oneshot_del_unit(adc_handle_);
        adc_handle_ = nullptr;
    }
    inited_ = false;
}

} // namespace sensors
