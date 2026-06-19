#include "common.h"
#include "lcd2.h"

int is_sleeping = 0;
int is_waveform = 0;

void lcd2_init(void)
{

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
	st7735_fill_screen(ST7735_BLACK);
	
	
}

void drawBlock(char *caption, int y, uint16_t color,  char *msg)
{
	st7735_fill_rect(0,y,20,11,color);
	// st7735_fill_rect(20,y,140,11,0);
	st7735_draw_string(2, y+2, caption, ST7735_WHITE, color, 1);
	st7735_draw_string(25, y+2, msg, ST7735_WHITE, 0, 1);
}

void lcd2_update()
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

void lcd2_sleep(int state)
{
	printf("LCD SLEEP %d\n",state);
	is_sleeping = state;
	st7735_sleep(state);

	if(!state)
		lcd2_update();
}


void lcd2_waveform(uint16_t *buf, int samples, int toggle)
{
	st7735_fill_screen(0);
	if(toggle && is_waveform)
	{
		is_waveform = 0;
		lcd2_update();
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
