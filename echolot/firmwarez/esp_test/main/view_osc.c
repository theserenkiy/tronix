#include "view_osc.h"
#include <sys/stat.h>

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

char str[WAV_INFO_SZ];
typedef struct {
	char type;
	uint16_t pings;
	uint16_t f0;
	uint16_t f1;
	uint16_t dur;
} cfg_short_t;

cfg_short_t cfgs[64];
cfg_short_t *ping_cfgs[128];
cfg_short_t *pc;
int cfglen = 0;
int ping_c_len = 0;
void parse_header(FILE *fp)
{
	fseek(fp, 0x38, SEEK_SET);
	fread(str,1,WAV_INFO_SZ,fp);
	char *ptr = str;
	str[WAV_INFO_SZ-1]=0;
	// printf("META %s\n",str);

	char typ;
	int f0,f1,dur,sdur,pings;
	cfglen = 0;
	cfg_short_t *c;
	int ret;
	while ((ptr = strstr(ptr, "\n")))
	{
		// printf("NL found str=%p ptr=%p char=|%c|\n",str,ptr,*ptr);
		if(ptr >= str+WAV_INFO_SZ-1)
		{
			printf("Break\n");
			break;
		}
		ptr++;
		if((*ptr == 'L' || *ptr == 'F' || *ptr == 'P') && *(ptr+1)==' ')
		{
			ret = sscanf(ptr,"%c %d %d %d %d %d",&typ,&pings,&f0,&f1,&dur,&sdur);
			if(ret && pings)
			{
				c = &cfgs[cfglen];
				c->type = typ;
				c->pings = pings;
				c->f0 = f0;
				c->f1 = f1;
				c->dur = sdur ? sdur : dur;
				cfglen++;
			}
		}
	}
	
	ping_c_len = 0;
	for(int i=0; i < cfglen; i++)
	{
		c = &cfgs[i];
		// printf("%c %d %d %d %d\n",c->type,c->pings,c->f0,c->f1,c->dur);
		for(int j=0; j < c->pings; j++)
		{
			ping_cfgs[ping_c_len++] = c;
		}
	}

}

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

		char path[64];
		snprintf(path, 64, "/sdcard/%s", DSTAT->last_filename);
		
		pf->num = DSTAT->last_filenum;
		pf->pingidx = 0;
		ping_c_len = 0;

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
			parse_header(pf->fp);
			fseek(pf->fp,sizeof(WAVHeader),SEEK_SET);
			printf("WAV header seeked\n");
		}
	}
}


void view_osc_update()
{
	printf("View osc update\n");
	// lcd_color_test();

	printf("Reading WAV file...\n");
	
	if(*pf->error)
	{
		ui_show_error(pf->error);
	}
	else
	{
		fseek(pf->fp,sizeof(WAVHeader) + ADC_RECORD_SAMPLES*2*pf->pingidx,SEEK_SET);
		fread(sonar_buffer,2,ADC_RECORD_SAMPLES,pf->fp);
		printf("DSP process...\n");
		dsp_process_raw_ping();
		printf("Draw OSC...\n");
		lcd_draw_osc(ADC_RECORD_SAMPLES);
		printf("OSC OK\n");
		// lcd_redraw();
	}

	lcd_text_format(1,COL_WHITE,COL_DGREEN,2);
	lcd_origin(0,0);
	printf("PF NUM %d\n",pf->num);
	printf("Q\n");
	lcd_wl("%d",pf->num);
	lcd_stack_right(0,0);
	lcd_text_format(1,COL_BLACK,COL_YELLOW,2);
	lcd_wl("%d / %d",pf->pingidx,pf->total_pings);

	if(pf->pingidx < ping_c_len)
	{
		lcd_stack_right(0,0);
		lcd_text_format(1,COL_WHITE,COL_BLACK,2);
		pc = ping_cfgs[pf->pingidx];
		lcd_wl("%c %d/%dk %du",pc->type,pc->f0,pc->f1,pc->dur);
	}
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