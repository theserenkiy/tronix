#include "common.h"
#include "driver/pulse_cnt.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "encoder.h"
#include "esp_timer.h"
#include "esp_rom_sys.h"

pcnt_unit_handle_t pcnt_unit = NULL;

static int pcnt_count = 0;
static int count = 0;

void encoder_init()
{
	gpio_config_t io_conf_a = {
        .intr_type = GPIO_INTR_DISABLE,
        .pin_bit_mask = (1ULL << ENC_A_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE
    };
    gpio_config(&io_conf_a);

    // 2. Конфигурация пина B (просто чтение уровня)
    gpio_config_t io_conf_b = {
        .intr_type = GPIO_INTR_DISABLE,
        .pin_bit_mask = (1ULL << ENC_B_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE
    };
    gpio_config(&io_conf_b);

	// Конфигурация счетчика
	pcnt_unit_config_t unit_config = {.high_limit = 10000, .low_limit = -10000};
	ESP_ERROR_CHECK(pcnt_new_unit(&unit_config, &pcnt_unit));

	// Настройка каналов для квадратурного режима
	pcnt_chan_config_t chan_a_config = {.edge_gpio_num = ENC_A_PIN, .level_gpio_num = ENC_B_PIN};
	pcnt_channel_handle_t pcnt_chan_a = NULL;
	ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &chan_a_config, &pcnt_chan_a));

	pcnt_chan_config_t chan_b_config = {.edge_gpio_num = ENC_B_PIN, .level_gpio_num = ENC_A_PIN};
	pcnt_channel_handle_t pcnt_chan_b = NULL;
	ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &chan_b_config, &pcnt_chan_b));

	// Установка логики счета
	ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_chan_a, PCNT_CHANNEL_EDGE_ACTION_DECREASE, PCNT_CHANNEL_EDGE_ACTION_INCREASE));
	ESP_ERROR_CHECK(pcnt_channel_set_level_action(pcnt_chan_a, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));
	// Аналогично для канала B...

	// Фильтр от дребезга и запуск
	pcnt_glitch_filter_config_t filt = {.max_glitch_ns = 10000};
	ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(pcnt_unit, &filt));
	ESP_ERROR_CHECK(pcnt_unit_enable(pcnt_unit));
	ESP_ERROR_CHECK(pcnt_unit_start(pcnt_unit));

}

void encoder_on_change(int direction)
{
	ui_on_encoder(direction);
}

void encoder_task(void *prm)
{
	encoder_loop();
}

void encoder_loop()
{
	int last_pcnt_count = 0, last_count = 0;
	int n = 0;
	uint8_t q = 0;
	int8_t sub,inc;
	while (1)
	{
		delay_ms(50);

		if(DSTAT->is_measuring)
			continue;

		pcnt_unit_get_count(pcnt_unit, &pcnt_count);
		q <<= 1;
		q &= 0x0F;
		sub = pcnt_count - last_pcnt_count;
		if(sub)
		{
			q |= 1;
			if(q==0b0011 || q==0b0101)
				inc = 0;
			else if(q==0b0111)
				inc = 2;
			else
				inc = 1;

			count += sub < 0 ? -inc : inc;
			last_pcnt_count = pcnt_count;

			if(count != last_count)
				encoder_on_change(count > last_count ? 1 : -1);
				

			last_count = count;


			// printf("N: %d; Encoder: %d, pcnt: %d; sub: %d; ABCD: " BYTE_TO_BINARY_PATTERN "\n",n,count,pcnt_count,sub,BYTE_TO_BINARY(q));
			// printf("Rec %d:\n",count);
			// printf("A: %s\n", record_A);
			// printf("B: %s\n", record_B);
		}

		
		n++;
	}
}