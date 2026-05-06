#pragma once

#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_err.h"
#include <cstdint>

namespace sensors {

/**
 * Датчик алкоголя MQ-3.
 *
 * Heater на GPIO27 (active HIGH).
 * Sensor на GPIO25 = ADC2_CHANNEL_8.
 *
 * ВАЖНО: ADC2 недоступен при включённом WiFi!
 * Читать только после отключения WiFi.
 */
class MQ3Sensor {
public:
    /**
     * @param heater_gpio    GPIO нагревателя (active high)
     * @param adc_channel    ADC канал (ADC2_CHANNEL_8 для GPIO25)
     */
    MQ3Sensor(gpio_num_t heater_gpio, adc_channel_t adc_channel,
              adc_unit_t adc_unit = ADC_UNIT_2);

    /** Инициализировать. */
    esp_err_t init();

    /** Включить нагреватель (нужно ~30 сек прогрева). */
    void heatOn();

    /** Выключить нагреватель. */
    void heatOff();

    /**
     * Считать сырое значение АЦП (12 бит, 0-4095).
     * Большее значение = больше алкоголя.
     */
    uint16_t readRaw();

    /** Проверить готовность (нагреватель работает). */
    bool isHeating() const { return heating_; }

    /** Деинициализация. */
    void deinit();

private:
    gpio_num_t    heater_gpio_;
    adc_channel_t adc_channel_;
    adc_unit_t    adc_unit_;
    bool          heating_  = false;
    bool          inited_   = false;
    adc_oneshot_unit_handle_t adc_handle_ = nullptr;
};

} // namespace sensors
