#include <stdio.h>
#include <inttypes.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

#define COLOR(C)(((C >> 16) & 0xF8)  | ((C << 3) & 0xE000) | ((C >> 13) & 0x7) | ((C << 5) & 0x1F00))

uint16_t sonar_buffer[20000];

uint16_t lcd_mk_color(int C)
{
	return ((C >> 16) & 0xF8)  | ((C << 3) & 0xE000) | ((C >> 13) & 0x7) | ((C << 5) & 0x1F00);
}

void lcd_draw_osc(int len)
{
	int16_t *buf = (uint16_t *)sonar_buffer;
	int min = 2047;
	int max = -2048;
	for(int i=0; i < len; i++)
	{
		if(max < buf[i])
			max = buf[i];
		if(min > buf[i])
			min = buf[i];
	}
	
	int amax = (abs(max) > abs(min)) ? abs(max) : abs(min);
	printf("Min: %d; Max: %d; Absmax: %d\n",min,max,amax);

	float vscale = amax/40.0;
	int hscale = len/160;

	printf("Vscale: %.3f; Hscale: %d; \n",vscale,hscale);

	uint16_t col[80];
	int colnum = 0;
	int colidx;
	uint16_t fill,maxfill;
	uint16_t vline[80];
	int vlidx;
	float k_br;
	uint8_t bright;
	uint16_t x = 0;
	for(int startsamp=0; startsamp < len; startsamp+=hscale)
	{
		memset(col,0,sizeof(col));
		int stopsamp = startsamp+hscale;
		for(int samp=startsamp; samp < stopsamp; samp++)
		{
			colidx = (int)((buf[samp]+amax)/vscale);
			// printf("Samp: %d; colidx: %d\n",samp,colidx);
			col[colidx]++;
		}
		
		fill = 0; 
		maxfill = 0;
		for(int i=0; i < 40; i++)
		{
			if(col[i])
			{
				fill += col[i];
				if(maxfill < fill)
					maxfill = fill;
			}
			col[i] = fill;
		}
		fill = 0;
		for(int i=79; i >=40; i--)
		{
			if(col[i])
			{
				fill += col[i];
				if(maxfill < fill)
					maxfill = fill;
			}
			col[i] = fill;
		}

		k_br = 32.0/maxfill;

		for(int i=0; i < 80; i++)
		{
			bright = (int)(col[i]*k_br) << 3;
			vline[79-i] = bright << 16 | bright << 8 | bright;//lcd_mk_color();
			// st7735_vline_buf(vline,x);
		}

		x++;
	}	

	for(int i=0; i < 80; i++)
	{
		printf("%x\n",vline[i]);
	}
}

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


void dsp_read_wav(char *fname, int pinglen_samp, int ping_idx)
{
	FILE *fp = fopen(fname, "rb");
	fseek(fp, 1088+(pinglen_samp*2*ping_idx), SEEK_SET);
	fread(sonar_buffer,2,pinglen_samp,fp);
	fclose(fp);

	int halfsz = pinglen_samp >> 1;
	uint16_t dc = mean(sonar_buffer+halfsz,halfsz,2048);
	printf("DC %d\n",dc);
	int16_t *intbuf = (int16_t *)sonar_buffer;
	sub_dc(intbuf, pinglen_samp, dc);
}

int main()
{
	dsp_read_wav("test.wav",12500,0);
	lcd_draw_osc(10000);

	return 0;
}