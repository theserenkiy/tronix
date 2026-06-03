#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s_std.h"
#include "esp_err.h"
#include "driver/pulse_cnt.h"

#define PCNT_HIGH_LIMIT 30000
#define PCNT_LOW_LIMIT -30000

// Задаем только ОДИН нужный пин для вывода меандра
#define OUTPUT_MEANDR_PIN    13 

#define TARGET_FREQ			3000000
#define SAMPLE_RATE			TARGET_FREQ / 64

#define SENSOR_PIN			27

pcnt_unit_handle_t pcnt_unit = NULL;
int current_total_pulses = 0;
int last_total_pulses = 0;

int32_t overflow_accumulator = 0; // Накопитель переполнений
int ovf_count = 0;

static bool pcnt_on_reach(pcnt_unit_handle_t unit, const pcnt_watch_event_data_t *edata, void *user_ctx) 
{
	BaseType_t high_task_wakeup = pdFALSE;

	ovf_count++;
	
	// Проверяем, достигнут ли верхний предел
	if (edata->watch_point_value == PCNT_HIGH_LIMIT) {
		(*(int32_t*)user_ctx) += PCNT_HIGH_LIMIT;
	} 
	// Проверяем, достигнут ли нижний предел
	else if (edata->watch_point_value == PCNT_LOW_LIMIT) {
		(*(int32_t*)user_ctx) += PCNT_LOW_LIMIT;
	}
	
	return (high_task_wakeup == pdTRUE);
}

void pcnt_init()
{
	printf("Инициализация аппаратного счетчика PCNT (IDF v6.0)...");
	
	// Конфигурация
	pcnt_unit_config_t unit_config = {
		.low_limit = PCNT_LOW_LIMIT,
		.high_limit = PCNT_HIGH_LIMIT,
		.flags.accum_count = true
	};

	ESP_ERROR_CHECK(pcnt_new_unit(&unit_config, &pcnt_unit));

	// // Регистрация колбэка
	// pcnt_event_callbacks_t cbs = {
	// 	.on_reach = pcnt_on_reach,
	// };
	// ESP_ERROR_CHECK(pcnt_unit_register_event_callbacks(pcnt_unit, &cbs, &overflow_accumulator));

	pcnt_unit_add_watch_point(pcnt_unit, PCNT_LOW_LIMIT);
	pcnt_unit_add_watch_point(pcnt_unit, PCNT_HIGH_LIMIT);

	pcnt_event_callbacks_t cbs = {
		.on_reach = pcnt_on_reach, // Сам колбэк нам не нужен, внутренний счетчик драйвера обновится автоматически
	};
	// Регистрируем структуру (последний аргумент NULL, так как пользовательские данные не передаем)
	ESP_ERROR_CHECK(pcnt_unit_register_event_callbacks(pcnt_unit, &cbs, &overflow_accumulator));

	// 2. Настройка аппаратного фильтра помех (глитчей)
	// Игнорируем импульсы короче 1000 наносекунд (1 микросекунда)
	// pcnt_glitch_filter_config_t filter_config = {
	//     .max_glitch_ns = 100,
	// };
	// ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(pcnt_unit, &filter_config));

	// 3. Создание и настройка канала для этого юнита
	pcnt_chan_config_t chan_config = {
		.edge_gpio_num = SENSOR_PIN,
		.level_gpio_num = -1, // Нам не нужен управляющий пин направления счета
	};
	pcnt_channel_handle_t pcnt_chan = NULL;
	ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &chan_config, &pcnt_chan));

	// 4. Задаем поведение счетчика на фронты сигнала
	// Увеличиваем счет (INCREASE) при нарастающем фронте (RISING edge)
	// Игнорируем спадающий фронт (HOLD на FALLING edge)
	ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_chan, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_HOLD));

	

	// 5. Включение и запуск счетчика
	ESP_ERROR_CHECK(pcnt_unit_enable(pcnt_unit));
	ESP_ERROR_CHECK(pcnt_unit_clear_count(pcnt_unit));
	ESP_ERROR_CHECK(pcnt_unit_start(pcnt_unit));

	printf("Счетчик запущен. Начинаем измерение частоты на GPIO %d...\n", SENSOR_PIN);

}

void pcnt_calc()
{
	vTaskDelay(1000 / portTICK_PERIOD_MS);

	ESP_ERROR_CHECK(pcnt_unit_get_count(pcnt_unit, &current_total_pulses));
	
	// Считать разницу БЕЗ сброса счетчика (исключает потерю импульсов в момент сброса)
	int pulses_in_this_second = current_total_pulses - last_total_pulses;
	last_total_pulses = current_total_pulses;


	printf("Текущая частота: %d Гц; ovf_count: %d\n", pulses_in_this_second, ovf_count);
}


void app_main(void)
{
	pcnt_init();

	i2s_chan_handle_t tx_handle = NULL;

	// Шаг 1: Выделяем ресурсы под TX канал (работаем как Мастер)
	i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
	
	// ВАЖНО: Нам не нужен DMA, обнуляем буферы, чтобы сэкономить RAM чипа
	chan_cfg.dma_desc_num = 2; 
	chan_cfg.dma_frame_num = 64;
	
	ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &tx_handle, NULL));

	// Шаг 2: Настраиваем конфигурацию стандартного режима
	i2s_std_config_t std_cfg = {
		// Задаем базовую частоту дискретизации (Sample Rate)
		.clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG((int) SAMPLE_RATE), 
		.slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_STEREO),
		.gpio_cfg = {
			.mclk = I2S_GPIO_UNUSED, // Наш целевой пин для меандра
			.bclk = OUTPUT_MEANDR_PIN,   // Игнорируем и оставляем свободным
			.ws   = I2S_GPIO_UNUSED,   // Игнорируем и оставляем свободным
			.dout = I2S_GPIO_UNUSED,   // Игнорируем и оставляем свободным
			.din  = I2S_GPIO_UNUSED,   // Игнорируем и оставляем свободным
			.invert_flags = {
				.mclk_inv = false,
			},
		},
	};

	// Шаг 3: Переключаем источник тактирования на высокоточный Audio PLL (APLL)
	std_cfg.clk_cfg.clk_src = I2S_CLK_SRC_APLL;

	/* Шаг 4: Магия математики частоты MCLK.
	 * По умолчанию MCLK считается как: Sample_Rate * MCLK_Multiple.
	 * Максимально стабильный и стандартный множитель — 64.
	 * Чтобы получить ровно 3 000 000 Гц на MCLK, нам нужна «виртуальная»
	 * аудио-частота дискретизации: 3 000 000 / 64 = 46 875 Гц.
	 */
	std_cfg.clk_cfg.sample_rate_hz = (int) SAMPLE_RATE;
	std_cfg.clk_cfg.mclk_multiple = I2S_MCLK_MULTIPLE_128;

	// Шаг 5: Инициализируем геометрию шины и запускаем тактирование
	ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_handle, &std_cfg));
	
	

	printf("Генерация меандра 3 МГц на GPIO %d успешно запущена через APLL MCLK!\n", OUTPUT_MEANDR_PIN);

	int gen_state = 0;
	while (1) {
		gen_state = !gen_state;
		if(gen_state)
			i2s_channel_enable(tx_handle);
		else
			i2s_channel_disable(tx_handle);

		// vTaskDelay(pdMS_TO_TICKS(1000));
		pcnt_calc();
	}
}
