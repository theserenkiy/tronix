#include <stdio.h>
#include <math.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "chirp.h"

// Настройки SPI
#define CHIRP_SPI_HOST     SPI3_HOST
#define PIN_NUM_MOSI       5     // Сигнал чирпа выйдет на этот пин!
#define SPI_CLOCK_HZ       10000000 // 10 МГц тактовая частота SPI (1 бит = 100 нс)

// Параметры чирпа
#define CHIRP_DURATION_US  250    // Длительность чирпа (250 мкс)
#define FREQ_START         178000.0 // 178 кГц
#define FREQ_END           190000.0 // 190 кГц

#define _2PI	2.0 * M_PI

// Буфер в DMA-способной памяти
uint8_t chirp_buffer[1024];
int chirp_bits = 0;

spi_device_handle_t spi_device;

void chirp_init() {
	// Конфигурация шины SPI
	spi_bus_config_t buscfg = {
		.mosi_io_num = PIN_NUM_MOSI,
		.miso_io_num = -1,
		.sclk_io_num = -1, // Клок не нужен, нам нужна только линия данных
		.quadwp_io_num = -1,
		.quadhd_io_num = -1,
		.max_transfer_sz = BUFFER_SIZE_BYTES
	};

	// Инициализируем SPI с включенным DMA (автовыбор канала 1 или 2)
	ESP_ERROR_CHECK(spi_bus_initialize(CHIRP_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO));

	// Конфигурация устройства на шине
	spi_device_interface_config_t devcfg = {
		.clock_speed_hz = SPI_CLOCK_HZ,
		.mode = 0,
		.spics_io_num = -1, // CS пин не используем
		.queue_size = 1,
		.flags = SPI_DEVICE_BIT_LSBFIRST // Меняем по вкусу, тут MSB/LSB
	};

	ESP_ERROR_CHECK(spi_bus_add_device(CHIRP_SPI_HOST, &devcfg, &spi_device));
}



// Функция заполнения буфера битовой маской ЛЧМ
void chirp_gen(int freq_start, int freq_end, int duration_us) {
	// Очищаем буфер

	int bits = ((SPI_CLOCK_HZ * duration_us) / 1000000);
	int buf_size = ((chirp_bits + 7) / 8);
	chirp_bits = buf_size * 8;

	// double t_duration = duration_us / 1000000.0;		// в секундах
	// double k = (freq_end - freq_start) / t_duration;	// скорость изменения частоты
	double dt = 1.0 / SPI_CLOCK_HZ;						// шаг времени одного бита (100 нс)

	double phase = 0.0;
	uint8_t byte;
	double dp = _2PI * freq_start * dt;
	double ddp = (dp - (_2PI * freq_end * dt))/chirp_bits;

	int big_ind = 0;

	for(int byte_num = 0; byte_num < buf_size; byte_num++)
	{
		byte = 0;
		for(int bit_num = 7; bit_num >= 0; bit_num--)
		{
			if (sin(phase) >= 0.0) {
				byte |= 1 << bit_num;
				// bigbuf[big_ind] = 1;
			}
			dp -= ddp;
			phase += dp;
			// big_ind++;
		}
		chirp_buffer[byte_num] = byte;
	}
}


// Функция мгновенного запуска излучения чирпа
void chirp_fire() {
	spi_transaction_t t;
	memset(&t, 0, sizeof(t));
	t.length = chirp_bits;           // Длина трансляции в битах
	t.tx_buffer = chirp_buffer;      // Указатель на DMA буфер

	// Синхронная передача через DMA (процессор отдаст команду и подождет 250 мкс)
	spi_device_transmit(spi_device, &t);
}

