#include <stdio.h>
#include <math.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "gen.h"
#include "common.h"

// Настройки SPI
#define GEN_SPI_HOST	SPI3_HOST
#define PIN_NUM_MOSI	TX_GPIO_2     // Сигнал чирпа выйдет на этот пин!
#define SPI_CLOCK_HZ	10000000 // 10 МГц тактовая частота SPI (1 бит = 100 нс)
#define GEN_BUF_SZ		4096

#define _2PI	2.0 * M_PI

// Буфер в DMA-способной памяти
// 1 мс = 10000 бит = 1.2кБ
uint8_t gen_buffer[GEN_BUF_SZ];
int gen_bits = 0;


spi_device_handle_t spi_device;

void gen_init() {
	// Конфигурация шины SPI
	spi_bus_config_t buscfg = {
		.mosi_io_num = PIN_NUM_MOSI,
		.miso_io_num = -1,
		.sclk_io_num = -1, // Клок не нужен, нам нужна только линия данных
		.quadwp_io_num = -1,
		.quadhd_io_num = -1,
		.max_transfer_sz = 2048
	};

	// Инициализируем SPI с включенным DMA (автовыбор канала 1 или 2)
	ESP_ERROR_CHECK(spi_bus_initialize(GEN_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO));

	// Конфигурация устройства на шине
	spi_device_interface_config_t devcfg = {
		.clock_speed_hz = SPI_CLOCK_HZ,
		.mode = 0,
		.spics_io_num = -1, // CS пин не используем
		.queue_size = 1,
		// .flags = SPI_DEVICE_BIT_MSBFIRST 
	};

	ESP_ERROR_CHECK(spi_bus_add_device(GEN_SPI_HOST, &devcfg, &spi_device));
}

void gen_clear_buffer()
{
	memset(gen_buffer,0,GEN_BUF_SZ);
}

// Функция заполнения буфера битовой маской ЛЧМ
void gen_psk(int freq, float sym_dur, float trans_dur, uint16_t data_bits, int sym_len) {
	// Очищаем буфер

	gen_clear_buffer();

	float half_trans_dur = trans_dur/2;
	float clean_dur = sym_dur-trans_dur;
	float T0 = 1.0/freq;
	int ftrans = (trans_dur+T0/2.0)/(T0*trans_dur);

	printf("F1: %d\n",ftrans);	

	double phase = 0.0;
	int bit_num = 0;
	int val,prevval=-1;
	gen_bits = 0;
	int chfreq;
	int endoffs = 15-sym_len+1;
	for(int symoffs=15; symoffs >= endoffs; symoffs--)
	{
		val = (data_bits >> symoffs) & 1;
		if(prevval < 0)
			prevval = val;
		// printf("%d",val);
		chfreq = (prevval == val) ? freq : ftrans;
		prevval = val;
		gen_bits += gen_chirp_continue(chfreq, chfreq, half_trans_dur, &phase, &bit_num);
		printf("Ph after trans: %.3f\n",phase/_2PI);
		gen_bits += gen_chirp_continue(freq, freq, clean_dur, &phase, &bit_num);
		printf("Ph before trans: %.3f\n",phase/_2PI);
		chfreq = (symoffs == endoffs || ((data_bits >> (symoffs-1)) & 1) == val) ? freq : ftrans;
		gen_bits += gen_chirp_continue(chfreq, chfreq, half_trans_dur, &phase, &bit_num);
	}
}

void gen_chirp(int freq_start, int freq_end, float duration)
{
	gen_clear_buffer();
	double phase = 0.0;
	int bit_num = 0;
	gen_bits = gen_chirp_continue(freq_start, freq_end, duration, &phase, &bit_num);
}

int gen_chirp_continue(int freq_start, int freq_end, float duration, double *phase, int *bit_num) {

	int bits = round(SPI_CLOCK_HZ * duration);

	double dt = 1.0 / SPI_CLOCK_HZ;						// шаг времени одного бита (100 нс)

	// double phase = 0.0;
	double dp = _2PI * freq_start * dt;
	double ddp = (dp - (_2PI * freq_end * dt))/bits;

	int endbit = (*bit_num) + bits;
	for(;(*bit_num) < endbit; (*bit_num)++)
	{
		if (sin(*phase) >= 0.0)
		{
			gen_buffer[(*bit_num)>>3] |= 1 << (7 - ((*bit_num) & 0x7));
			//bigbuf[big_ind] = 1;
		}
		else
		{
			//bigbuf[big_ind] = 0;
		}
		dp -= ddp;
		(*phase) += dp;
		//big_ind++;
	}
		
	return bits;
}

// Функция мгновенного запуска излучения чирпа
void gen_fire() {
	// printf("crp\n");
	spi_transaction_t t = {
		.length = gen_bits,
		.tx_buffer = gen_buffer
	};
	
	spi_device_transmit(spi_device, &t);
}

