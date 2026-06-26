#include "common.h"
#include "views.h"
#include <sys/stat.h>

void set_next_depth(int direction)
{
	int cur = DSTAT->depth_set_mm;
	if(direction < 0)
	{
		if(cur <= 1000)
			return;
		DSTAT->depth_set_mm -= cur <= 10000 ? 500 : 1000;
	}
	else
	{
		if(cur >= 40000)
			return;
		DSTAT->depth_set_mm += cur < 10000 ? 500 : 1000;
	}
	
}

/////////////////////////////////////////////////////////
// MAIN
void view_main_update()
{
	printf("View main update\n");
	lcd_origin(0,0);
	lcd_text_format(2,COLOR_WHITE,COLOR_DARKGREEN,1);
	lcd_wl("NXT FIL");	
	
	lcd_stack_right(4,0);
	lcdc->text_bg = 0;
	lcd_wl("%d",DSTAT->last_filenum+1);
	lcd_stack_down(0,10);
	lcdc->x = 10;
	lcd_text_format(4,COLOR(0xCCCCCC),COLOR(0x000055),6);
	lcd_wl(
		DSTAT->depth_set_mm >= 10000 ? "%.0fm" : "%.1fm",
		DSTAT->depth_set_mm/1000.0
	);

}

int view_main_on_event(event_t *ev)
{
	ui_print_event("View main received event",ev);
	if(ev->type == 'E')
	{
		set_next_depth(ev->value);
		ui_update_view();
	}
	else if(ev->type == 'B')
	{
		if(ev->value == 0)
		{
			ui_sleep(1);
			recorder_make_record();
			// delay_ms(2000);
			ui_sleep(0);
		}
	}
	return 0;
}

/////////////////////////////////////////////////////////
// OSC
typedef struct {
	int num;
	int size;
	int pingidx;
	int total_pings;
	FILE *fp;
	char error[16];
} pingfile_t;

pingfile_t pingfile = {
	.num = 0,
	.error = "Not inited",
	.fp = NULL
};

pingfile_t *pf = &pingfile;

void view_osc_init()
{
	if(!DSTAT->last_filenum)
	{
		strcpy(pf->error,"No files");
		return;
	}

	if(DSTAT->last_filenum != pf->num)
	{
		if(pf->fp)
		{
			printf("Close prev file\n");
			fclose(pf->fp);
		}

		char path[24];
		sprintf(path, "/sdcard/save_%06d.wav", DSTAT->last_filenum);
		
		pf->num = DSTAT->last_filenum;
		pf->pingidx = 0;

		struct stat st;
		stat(path, &st);
		pf->size = st.st_size;
		printf("Size: %d\n",pf->size);

		if(pf->size < (ADC_RECORD_SAMPLES << 1))
		{
			strcpy(pf->error,"Too short");
		}
		else{
			pf->total_pings = pf->size/2/ADC_RECORD_SAMPLES;
			*pf->error = 0;
			printf("Opening %s\n",path);
			pf->fp = fopen(path,"rb");
			printf("Opened\n");
			fseek(pf->fp,sizeof(WAVHeader),SEEK_SET);
			printf("WAV header seeked\n");
		}
	}
}


void view_osc_update()
{
	printf("View osc update\n");
	// lcd_color_test();

	printf("Reading...");
	
	if(*pf->error)
	{
		ui_show_error(pf->error);
	}
	else
	{
		fseek(pf->fp,sizeof(WAVHeader) + ADC_RECORD_SAMPLES*2*pf->pingidx,SEEK_SET);
		fread(sonar_buffer,2,ADC_RECORD_SAMPLES,pf->fp);
		dsp_process_raw_ping();
		lcd_draw_osc(ADC_RECORD_SAMPLES);
		lcd_redraw();
	}

	lcd_text_format(1,COLOR_WHITE,COLOR_DARKGREEN,2);
	lcd_origin(0,0);
	printf("PF NUM %d\n",pf->num);
	printf("Q\n");
	lcd_wl("%d",pf->num);
	lcd_stack_right(0,0);
	lcd_text_format(1,COLOR_BLACK,COLOR_YELLOW,2);
	lcd_wl("%d / %d",pf->pingidx,pf->total_pings);
}

int view_osc_on_event(event_t *ev)
{
	ui_print_event("View osc received event",ev);
	if(ev->type == 'E')
	{
		if(*pf->error)
			return 0;
		pf->pingidx += ev->value;
		if(pf->pingidx >= pf->total_pings)
			pf->pingidx = pf->total_pings-1;
		else if(pf->pingidx < 0)
			pf->pingidx = 0;

		ui_update_view();
	}
	return 0;
}