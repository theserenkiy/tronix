#include "common.h"
#include "gen.h"
#include "wav.h"
#include "dsp.h"

static uint16_t mean(uint16_t* buf, size_t len, uint16_t initial)
{
	double avg = (double)initial;
	for(int i=0; i < len; i++)
	{
		// printf("%d\n",buf[i]);
		avg += (buf[i] - avg)/(i+1);
	}
	return (uint16_t)avg;
}

static void sub_dc(int16_t* buf, size_t len, uint16_t dc)
{
	for(int i=0; i < len; i++)
	{
		buf[i] -= dc;
	}
}

void dsp_process_raw_ping()
{
	int halfsz = ADC_RECORD_SAMPLES >> 1;
	uint16_t dc = mean(sonar_buffer+halfsz,halfsz,2048);
	int16_t *intbuf = (int16_t *)sonar_buffer;
	sub_dc(intbuf, ADC_RECORD_SAMPLES, dc);
}

void dsp_read_wav(char *fname, int ping_idx)
{
	FILE *fp = fopen(fname, "rb");
	fseek(fp, sizeof(WAVHeader)+(ADC_RECORD_SAMPLES*2*ping_idx), SEEK_SET);
	fread(sonar_buffer,2,ADC_RECORD_SAMPLES,fp);
	fclose(fp);
	dsp_process_raw_ping();
}
