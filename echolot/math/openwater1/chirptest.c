#include <stdio.h>
#include <stdint.h>
#define _USE_MATH_DEFINES
#include <math.h>

#define SPI_CLOCK_HZ       10000
#define _2PI	2.0 * M_PI

uint16_t bigbuf[32768];
int big_ind = 0;

uint8_t gen_buffer[4096];
int gen_bits;

int gen_chirp(int freq_start, int freq_end, float duration, double *phase, int *bit_num);

// Функция заполнения буфера битовой маской ЛЧМ
int gen_psk(int freq, float sym_dur, float trans_dur, uint16_t data_bits, int sym_len) {
	// Очищаем буфер

	float half_trans_dur = trans_dur/2;
	float clean_dur = sym_dur-trans_dur;
	float T0 = 1.0/freq;
	int ftrans = (trans_dur+T0/2.0)/(T0*trans_dur);

	printf("F1: %d\n",ftrans);	

	double phase = 0.0;
	int bit_num = 0;
	int val,prevval=-1;
	int is_same = 1;
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
		gen_bits += gen_chirp(chfreq, chfreq, half_trans_dur, &phase, &bit_num);
		printf("Ph after trans: %.3f\n",phase/_2PI);
		gen_bits += gen_chirp(freq, freq, clean_dur, &phase, &bit_num);
		printf("Ph before trans: %.3f\n",phase/_2PI);
		chfreq = (symoffs == endoffs || ((data_bits >> (symoffs-1)) & 1) == val) ? freq : ftrans;
		gen_bits += gen_chirp(chfreq, chfreq, half_trans_dur, &phase, &bit_num);
	}
}

int gen_chirp(int freq_start, int freq_end, float duration, double *phase, int *bit_num) {

	int bits = round(SPI_CLOCK_HZ * duration);

	// printf("Chirp gen: %d -> %d; bits: %d; bitnum: %d\n",freq_start,freq_end,bits,*bit_num);
	// int buf_size = ((bits + 7) / 8);
	// int gen_bits = buf_size * 8;

	// double t_duration = duration_us / 1000000.0;		// в секундах
	// double k = (freq_end - freq_start) / t_duration;	// скорость изменения частоты
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
			bigbuf[big_ind] = 1;
		}
		else
		{
			bigbuf[big_ind] = 0;
		}
		dp -= ddp;
		(*phase) += dp;
		big_ind++;
	}
		
	return bits;
}

// int gen_chirp(int freq_start, int freq_end, float duration, double *phase, int *byte_num) {

// 	printf("Chirp gen: %d -> %d\n",freq_start,freq_end);
// 	int bits = (SPI_CLOCK_HZ * duration);
// 	int buf_size = ((bits + 7) / 8);
// 	int gen_bits = buf_size * 8;

// 	// double t_duration = duration_us / 1000000.0;		// в секундах
// 	// double k = (freq_end - freq_start) / t_duration;	// скорость изменения частоты
// 	double dt = 1.0 / SPI_CLOCK_HZ;						// шаг времени одного бита (100 нс)

// 	// double phase = 0.0;
// 	uint8_t byte;
// 	double dp = _2PI * freq_start * dt;
// 	double ddp = (dp - (_2PI * freq_end * dt))/gen_bits;

	
// 	int end_byte_num = *byte_num+buf_size;
// 	for(; *byte_num < end_byte_num; (*byte_num)++)
// 	{
// 		// printf("Byte: %d\n",*byte_num);
// 		byte = 0;
// 		for(int bit_num = 7; bit_num >= 0; bit_num--)
// 		{
// 			if (sin(*phase) >= 0.0) {
// 				byte |= 1 << bit_num;
// 				bigbuf[big_ind] = 1;
// 			}
// 			dp -= ddp;
// 			*phase += dp;
// 			big_ind++;
// 		}
// 		gen_buffer[*byte_num] = byte;
// 	}

// 	return gen_bits;
// }

int main()
{
	///gen_chirp(10,20,2e6);

	double phase = 0.0;
	int byte_num = 0;

	gen_chirp(200,100,0.2,&phase,&byte_num);
	gen_chirp(100,200,0.2,&phase,&byte_num);

	//gen_psk(200, 0.10, 0.040, 0b1011001011001010, 16);
	FILE *fp = fopen("chirp.bin", "wb");
	printf("Writing %d bytes\n",big_ind*2);
	fwrite(bigbuf,2,big_ind,fp);
	fclose(fp);

	return 0;
}