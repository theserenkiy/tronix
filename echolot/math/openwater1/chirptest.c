#include <stdio.h>
#include <stdint.h>
#define _USE_MATH_DEFINES
#include <math.h>

#define SPI_CLOCK_HZ       1000
#define _2PI	2.0 * M_PI

uint8_t chirp_buffer[1024];
int chirp_bits;

void chirp_gen(int freq_start, int freq_end, int duration_us) {
	// Очищаем буфер

	int bits = ((SPI_CLOCK_HZ * duration_us) / 1000000);
	int buf_size = ((bits + 7) / 8);
	chirp_bits = buf_size * 8;

	// double t_duration = duration_us / 1000000.0;		// в секундах
	// double k = (freq_end - freq_start) / t_duration;	// скорость изменения частоты
	double dt = 1.0 / SPI_CLOCK_HZ;						// шаг времени одного бита (100 нс)

	double phase = 0.0;
	uint8_t byte;
	double dp = _2PI * freq_start * dt;
	double ddp = (dp - (_2PI * freq_end * dt))/chirp_bits;

	// int big_ind = 0;

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
		printf("%d%d%d%d%d%d%d%d\n",
			byte & (1 << 7) >> 7,
			byte & (1 << 6) >> 6,
			byte & (1 << 5) >> 5,
			byte & (1 << 4) >> 4,
			byte & (1 << 3) >> 3,
			byte & (1 << 2) >> 2,
			byte & (1 << 1) >> 1,
			byte & 1
		);
	}
}

int main()
{
	chirp_gen(10,20,2e6);


	return 0;
}