#include <stdio.h>
#include <stdint.h>
#define _USE_MATH_DEFINES
#include <math.h>

#define SPI_CLOCK_HZ       1000
#define _2PI	2.0 * M_PI

uint8_t chirp_buffer[1024];
uint8_t bigbuf[8192];

// Функция заполнения буфера битовой маской ЛЧМ
int chirp_generate_bitmap(int freq_start, int freq_end, int duration_us) {
	// Очищаем буфер

	int total_bits = ((SPI_CLOCK_HZ * duration_us) / 1000000);
	int buf_size = ((total_bits + 7) / 8);

	// double t_duration = duration_us / 1000000.0;		// в секундах
	// double k = (freq_end - freq_start) / t_duration;	// скорость изменения частоты
	double dt = 1.0 / SPI_CLOCK_HZ;						// шаг времени одного бита (100 нс)


	double phase = 0.0;
	uint8_t byte;
	double dp = _2PI * freq_start * dt;
	double ddp = (dp - (_2PI * freq_end * dt))/total_bits;

	int big_ind = 0;

	for(int byte_num = 0; byte_num < buf_size; byte_num++)
	{
		byte = 0;
		for(int bit_num = 7; bit_num >= 0; bit_num--)
		{
			// phase = _2PI * (freq_start * t + (k / 2.0) * t * t);
			if (sin(phase) >= 0.0) {
				byte |= 1 << bit_num;
				bigbuf[big_ind] = 1;
			}
			else
			{
				bigbuf[big_ind] = 0;
			}
			// t += dt;
			dp -= ddp;
			phase += dp;
			big_ind++;
		}
		chirp_buffer[byte_num] = byte;
	}

	return buf_size;
}

int main()
{
	int bytes = chirp_generate_bitmap(10,20,2e6);

	printf("Bytes: %d\n",bytes);

	FILE *fp = fopen("chirp.bin","wb");
	fwrite(bigbuf,1,bytes*8,fp);
	fclose(fp);

	return 0;
}