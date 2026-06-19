#include "sonar_tx.h"
#include "driver/mcpwm_prelude.h"



static mcpwm_timer_handle_t timer_1 = NULL;
static mcpwm_oper_handle_t oper = NULL;
static mcpwm_cmpr_handle_t comparator = NULL;
static mcpwm_gen_handle_t generator_1 = NULL;
static mcpwm_gen_handle_t generator_2 = NULL;


void sonar_tx_init(void)
{
	gpio_reset_pin(MOSDRV_ENA_PIN);
	gpio_set_direction(MOSDRV_ENA_PIN, GPIO_MODE_OUTPUT);
	gpio_set_level(MOSDRV_ENA_PIN, 0);

	gpio_reset_pin(DCDC_ENA_PIN);
	gpio_set_direction(DCDC_ENA_PIN, GPIO_MODE_OUTPUT);
	gpio_set_level(DCDC_ENA_PIN, 1);

	mcpwm_timer_config_t timer_config = {
		.group_id = 0,
		.clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
		.resolution_hz = 40000000,     // 40 МГц
		.period_ticks = 40000000 / TX_FREQ_HZ,
		.count_mode = MCPWM_TIMER_COUNT_MODE_UP,
	};

	ESP_ERROR_CHECK(mcpwm_new_timer(&timer_config, &timer_1));

	mcpwm_operator_config_t oper_config = {
		.group_id = 0,
	};

	ESP_ERROR_CHECK(mcpwm_new_operator(&oper_config, &oper));
	ESP_ERROR_CHECK(mcpwm_operator_connect_timer(oper, timer_1));

	mcpwm_comparator_config_t cmp_config = {
		.flags.update_cmp_on_tez = true,
	};

	ESP_ERROR_CHECK(mcpwm_new_comparator(oper, &cmp_config, &comparator));

	mcpwm_generator_config_t gen_config_1 = {
		.gen_gpio_num = TX_GPIO_1,
	};

	ESP_ERROR_CHECK(mcpwm_new_generator(oper, &gen_config_1, &generator_1));

	mcpwm_generator_config_t gen_config_2 = {
		.gen_gpio_num = TX_GPIO_2,
	};

	ESP_ERROR_CHECK(mcpwm_new_generator(oper, &gen_config_2, &generator_2));

	uint32_t period = 40000000 / TX_FREQ_HZ;

	ESP_ERROR_CHECK(
		mcpwm_comparator_set_compare_value(
			comparator,
			period / 2
		)
	);

	ESP_ERROR_CHECK(
		mcpwm_generator_set_action_on_timer_event(
			generator_1,
			MCPWM_GEN_TIMER_EVENT_ACTION(
				MCPWM_TIMER_DIRECTION_UP,
				MCPWM_TIMER_EVENT_EMPTY,
				MCPWM_GEN_ACTION_HIGH
			)
		)
	);

	ESP_ERROR_CHECK(
		mcpwm_generator_set_action_on_compare_event(
			generator_1,
			MCPWM_GEN_COMPARE_EVENT_ACTION(
				MCPWM_TIMER_DIRECTION_UP,
				comparator,
				MCPWM_GEN_ACTION_LOW
			)
		)
	);

	ESP_ERROR_CHECK(
		mcpwm_generator_set_action_on_timer_event(
			generator_2,
			MCPWM_GEN_TIMER_EVENT_ACTION(
				MCPWM_TIMER_DIRECTION_UP,
				MCPWM_TIMER_EVENT_EMPTY,
				MCPWM_GEN_ACTION_LOW
			)
		)
	);

	ESP_ERROR_CHECK(
		mcpwm_generator_set_action_on_compare_event(
			generator_2,
			MCPWM_GEN_COMPARE_EVENT_ACTION(
				MCPWM_TIMER_DIRECTION_UP,
				comparator,
				MCPWM_GEN_ACTION_HIGH
			)
		)
	);

	ESP_ERROR_CHECK(mcpwm_timer_enable(timer_1));
}

void sonar_precharge(int ms)
{
	sonar_charge(1);
	vTaskDelay(pdMS_TO_TICKS(ms));
}

void sonar_charge(int state)
{
	gpio_set_level(DCDC_ENA_PIN, state);
}

void sonar_tx_burst(uint32_t cycles, int need_osc_suppression)
{
	sonar_charge(1);
	vTaskDelay(pdMS_TO_TICKS(MT_PRECHARGE_DELAY_MS));

	gpio_set_level(MOSDRV_ENA_PIN, 1);

    uint32_t burst_us =
        (cycles * 1000000ULL) / TX_FREQ_HZ;

	mcpwm_generator_set_force_level(generator_1, -1, true);
	mcpwm_generator_set_force_level(generator_2, -1, true);

	mcpwm_timer_start_stop(
		timer_1,
		MCPWM_TIMER_START_NO_STOP
	);
	// ESP_LOGI("TX", "start=%s", esp_err_to_name(err));

    esp_rom_delay_us(burst_us);

    mcpwm_timer_start_stop(
        timer_1,
        MCPWM_TIMER_STOP_EMPTY
    );

    mcpwm_generator_set_force_level(generator_1, 0, true);
	mcpwm_generator_set_force_level(generator_2, 0, true);

	if(need_osc_suppression)
		esp_rom_delay_us(IR_DSBL_DELAY_US);
		
	gpio_set_level(MOSDRV_ENA_PIN, 0);
	sonar_charge(0);
}