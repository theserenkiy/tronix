#include "common.h"
#include "lcd.h"
#include "math.h"

int is_sleeping = 0;
int is_waveform = 0;

void lcd_init(void)
{
	spi_mutex = xSemaphoreCreateMutex();

	gpio_reset_pin(LCD_LED_PIN);
	gpio_set_direction(LCD_LED_PIN, GPIO_MODE_OUTPUT);
	gpio_set_drive_capability(LCD_LED_PIN, GPIO_DRIVE_CAP_2);
	gpio_set_level(LCD_LED_PIN, 1);

	// Конфигурация дисплея
	st7735_config_t cfg = {
		.cs_io_num   = LCD_CS_PIN,
		.dc_io_num   = LCD_DC_PIN,
		.rst_io_num  = LCD_RST_PIN,
		.bl_io_num   = -1,  // -1 = не используется
		.host_id     = SPI2_HOST,     // используем SPI2
	};

	
	ESP_LOGI("ST7735", "Initializing display...");
	esp_err_t ret = st7735_init(&cfg);
	
	if (ret != ESP_OK) {
		ESP_LOGE("ST7735", "Display initialization failed!");
		return;
	}
	
	ESP_LOGI("ST7735", "Display initialized successfully");
	
	// Очистка экрана черным цветом
	st7735_fill_screen(ST7735_GREEN);
	// st7735_fill_rect(10,10,10,10,ST7735_RED);
	st7735_draw_pixel(10,10,ST7735_RED);
	st7735_draw_pixel(150,10,ST7735_BLUE);
	st7735_draw_pixel(150,70,ST7735_GREEN);
	st7735_draw_pixel(10,70,ST7735_YELLOW);
	st7735_draw_string(20,20,"Preved!",ST7735_RED,2);
	st7735_redraw();
}

inline void lcd_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
	st7735_fill_rect(x,y,w,h,color);
}

inline void lcd_fill_screen(uint16_t color)
{
	st7735_fill_screen(color);
}

inline void lcd_put_pixel(uint16_t x, uint16_t y, uint16_t color)
{
	st7735_draw_pixel(x, y, color);
}

inline void lcd_redraw()
{
	st7735_redraw();
}

uint16_t lcd_mk_color(int C)
{
	return ((C >> 16) & 0xF8)  | ((C << 3) & 0xE000) | ((C >> 13) & 0x7) | ((C << 5) & 0x1F00);
}

uint16_t lcd_mk_gray(uint8_t C)
{
	return (C & 0xF8) | (C >> 5) | ((C & 0xFC) << 11) | ((C & 0xF8) << 5);
}

uint16_t lcd_mk_hsl_color(uint16_t h, uint8_t s, float l)
{
	float _2PI = 2*M_PI;
	float _23PI = _2PI/3;
	float Rphase = _2PI*(h/360.0);
	float Gphase = Rphase+_23PI;
	float Bphase = Gphase+_23PI;

	return lcd_mk_color(
		(int)((s*sin(Rphase)+127)*l) << 16	|
		(int)((s*sin(Gphase)+127)*l) << 8	|
		(int)((s*sin(Bphase)+127)*l)
	);
}

// 0 < t < 32
uint16_t lcd_mk_temperature_color(uint8_t t)
{
	uint8_t steps = 32;
	uint8_t hmin = 40;
	uint8_t hmax = 170;
	float hstep = (float)(hmax-hmin)/steps;
	float minl = 0.4;
	float lstep = (1-minl)/steps;
	return lcd_mk_hsl_color((int)(hmax-(t*hstep)),127,1);//minl+(t*lstep));
}

void lcd_gray_test()
{
	for(int gray=0;gray < 32;gray++)
	{
		lcd_fill_rect(5*gray,0,5,80,lcd_mk_temperature_color(gray));
	}
	lcd_redraw();
}

void lcd_draw_osc(int len)
{
	lcd_fill_screen(0);
	uint16_t tempcolors[32];
	for(int i=0; i < 32; i++)
		tempcolors[i] = i ? lcd_mk_temperature_color(i) : 0;
	int16_t *buf = (int16_t *)sonar_buffer;
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
	int colidx;
	uint16_t fill,maxfill;
	uint16_t vline[80];
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

		k_br = 31.0/maxfill;

		for(int i=0; i < 80; i++)
		{
			bright = (int)(col[i]*k_br);
			vline[79-i] = tempcolors[bright];
			st7735_vline_buf(vline,x);
		}

		x++;
	}	

	// for(int i=0; i < 80; i++)
	// {
	// 	printf("%x\n",vline[i]);
	// }
}



void drawBlock(char *caption, int y, uint16_t color,  char *msg)
{
	st7735_fill_rect(0,y,20,11,color);
	// st7735_fill_rect(20,y,140,11,0);
	st7735_draw_string(2, y+2, caption, ST7735_WHITE, 1);
	st7735_draw_string(25, y+2, msg, ST7735_WHITE, 1);
}

void lcd_update()
{
	if(is_sleeping || is_waveform)
		return;
	printf("LCD UPDATE...\n");

	drawBlock(
		"TIM",
		0, 
		DSTAT->time_set && DSTAT->date_set ? ST7735_TURQUOSE : ST7735_RED,  
		DSTAT->datetime
	);
	char fnum[10];
	sprintf(fnum,"%d",DSTAT->filenum);
	drawBlock(
		"SD", 
		12, 
		DSTAT->sd_ok ? ST7735_DARKGREEN : ST7735_RED, 
		fnum
	);
	drawBlock(
		"GPS",
		24, 
		DSTAT->gps_enabled 
			? (DSTAT->gps_ok ?  ST7735_DARKBLUE : ST7735_RED)
			: ST7735_DARKORANGE, 
		DSTAT->gps_str
	);

	printf("LCD UPDATE OK\n");
}

void lcd_sleep(int state)
{
	printf("LCD SLEEP %d\n",state);
	is_sleeping = state;
	st7735_sleep(state);

	if(!state)
		lcd_update();
}


void lcd_waveform(uint16_t *buf, int samples, int toggle)
{
	st7735_fill_screen(0);
	if(toggle && is_waveform)
	{
		is_waveform = 0;
		lcd_update();
		return;
	}
	is_waveform = 1;
	
	float ysc = 51.2;
	float xsc = samples/160;
	int min,max,start,end;
	for(int x=0; x < 160; x++)
	{
		min=4095;
		max=0;
		start=(int)(x*xsc);
		end=(int)(start+xsc);
		for(int i=start; i < end; i++)
		{
			if(buf[i] > max)
				max = buf[i];
			if(buf[i] < min)
				min = buf[i];
		}
		max = (int)(max/ysc);
		min = (int)(min/ysc);
		printf("max %d min %d\n",max,min);
		st7735_fill_rect(x,80-max,1,max-min,0xFFFF);
	}
}
