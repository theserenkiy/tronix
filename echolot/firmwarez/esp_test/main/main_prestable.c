#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/rmt_tx.h"
#include "esp_log.h"

#define TX_GPIO_NUM         GPIO_NUM_13
#define BURST_DURATION_US   100   // Длина пачки 100 мкс
#define TOTAL_PERIOD_MS     50    // Период повторения 50 мс

static const char *TAG = "RMT_NEW_BURST";

void app_main(void)
{
	ESP_LOGI(TAG, "Initializing New RMT TX Channel...");

	
	// 1. Конфигурация нового TX канала
	rmt_channel_handle_t tx_chan = NULL;
	// rmt_tx_channel_config_t tx_chan_config = {0}; 
    
	rmt_tx_channel_config_t tx_chan_config = {
		.clk_src = RMT_CLK_SRC_DEFAULT,   // select source clock
		.gpio_num = TX_GPIO_NUM,                    // GPIO number
		.mem_block_symbols = 64,          // memory block size, 64 * 4 = 256 Bytes
		.resolution_hz = 1 * 1000 * 1000, // 1 MHz tick resolution, i.e., 1 tick = 1 µs
		.trans_queue_depth = 4,           // set the number of transactions that can pend in the background
		.flags.invert_out = false,        // do not invert output signal
		// .flags.with_dma = false,          // do not need DMA backend
	};

   
	ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config, &tx_chan));

	// 2. Настройка модуляции несущей (175 кГц, скважность 50%)
	rmt_carrier_config_t carrier_config = {
		.frequency_hz = 175000,
		.duty_cycle = 0.5f,
		.flags.polarity_active_low = false, // Модуляция активна при высоком уровне символа
	};
	ESP_ERROR_CHECK(rmt_apply_carrier(tx_chan, &carrier_config));

	// 3. Создаем базовый копирующий энкодер (Copy Encoder)
	rmt_encoder_handle_t copy_encoder = NULL;
	rmt_copy_encoder_config_t copy_encoder_config = {};
	ESP_ERROR_CHECK(rmt_new_copy_encoder(&copy_encoder_config, &copy_encoder));

	// Включаем канал
	ESP_ERROR_CHECK(rmt_enable(tx_chan));

	// 4. Описание структуры сигнала
	// В новом API символ задается структурой rmt_symbol_word_t (длительность в тиках + уровень)
	// Так как разрешение выставлено в 1 000 000 Гц, 1 единица длительности = 1 мкс.
	rmt_symbol_word_t burst_signal[] = {
		{
			// Символ 1: Пачка импульсов + первая часть паузы
			.duration0 = BURST_DURATION_US, // 100 мкс (активная пачка)
			.level0 = 1,
			.duration1 = 25000,             // 25000 мкс (низкий уровень)
			.level1 = 0
		},
		{
			// Символ 2: Вторая часть паузы (продолжаем держать низкий уровень)
			// Оставшееся время: 49900 - 25000 = 24900 мкс.
			// Делим 24900 на два плеча одного символа (например, 15000 + 9900)
			.duration0 = 15000,             // 15000 мкс (низкий уровень)
			.level0 = 0,
			.duration1 = 9900,              // 9900 мкс (низкий уровень)
			.level1 = 0
		},
        {
            // 3. Маркер конца потока для rmt_copy_encoder.
            // Нулевая длительность сообщает кодеру, что массив завершен.
            .duration0 = 0,
            .level0 = 0,
            .duration1 = 0,
            .level1 = 0
        }
	};

	// Конфигурация транзакции передачи
	rmt_transmit_config_t transmit_config = {
		.loop_count = -1, // Значение -1 означает бесконечный автоматический аппаратный цикл!
		// .eot_level = 0,             // Уровень линии при завершении/паузе — строго 0
        .flags.queue_nonblocking = false
	};

	ESP_LOGI(TAG, "Starting continuous burst generation...");
	
	// Запуск бесконечного цикла генерации силами железа
	ESP_ERROR_CHECK(rmt_transmit(tx_chan, copy_encoder, &burst_signal, sizeof(burst_signal) / sizeof(rmt_symbol_word_t), &transmit_config));

	while (1) {
		// Процессор полностью свободен, генерация пачек зациклена на аппаратном уровне
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}
