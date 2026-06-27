#include "common.h"
#include "views.h"


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
	lcd_text_format(1,COL_WHITE,COL_DGREEN,1);
	lcd_wl("NEXT FILE");	
	lcd_stack_right(4,0);
	lcdc->text_bg = 0;
	lcd_wl("%d",DSTAT->last_filenum+1);

	lcd_stack_down(0,1);
	lcdc->x = 0;
	lcd_text_format(1,COL_WHITE,COL_DORANGE,1);
	lcd_wl("TST CNF");	
	lcd_stack_right(4,0);
	lcdc->text_bg = 0;
	lcd_wl("%s",confparser_get_name());

	lcd_stack_down(0,10);
	lcdc->x = 10;
	lcd_text_format(4,COLOR(0xCCCCCC),COLOR(0x000055),6);
	lcd_wl(
		DSTAT->depth_set_mm >= 10000 ? "%.0fm" : "%.1fm",
		DSTAT->depth_set_mm/1000.0
	);

	lcd_origin(100,0);
	lcd_text_format(2,COL_WHITE,COL_BLACK,2);
	lcd_wl("%dk",DSTAT->center_freq_khz);

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



///////////////////////////////////////////////////////
// TESTCONF

void view_tconf_init()
{

}

void view_tconf_deinit()
{
	dump_saves();
}

void view_tconf_update()
{
	lcd_text_format(2,COL_WHITE, COL_DTURQ, 2);
	lcd_origin(10,0);
	lcd_wl("TEST");
	lcd_stack_down(0,2);
	lcdc->text_bg = 0;
	char *name = confparser_get_name();
	lcd_wl("%s",name);
	lcd_stack_down(0,4);
	lcdc->x = 0;
	lcd_text_format(1,COL_YELLOW,0,2);
	char str[16];
	confparser_test_summary(str);
	lcd_wl(str);
}

int view_tconf_on_event(event_t *ev)
{
	if(ev->type=='E')
	{
		printf("TEST encoder %d\n",ev->value);
		confparser_next(ev->value);
		printf("DONE\n");
		ui_update_view();
	}
	return 0;
}

/////////////////////////////////////////////////
// FREQ

///////////////////////////////////////////////////////
// TESTCONF

void view_freq_init()
{

}

void view_freq_deinit()
{
	dump_saves();
}

void view_freq_update()
{
	lcd_text_format(2,COL_BLACK, COL_GREEN, 2);
	lcd_origin(10,0);
	lcd_wl("FREQ");
	lcd_stack_down(0,2);
	lcd_text_format(2,COL_WHITE, COL_BLACK, 2);
	
	lcd_wl("%d kHz",DSTAT->center_freq_khz);
}

int view_freq_on_event(event_t *ev)
{
	if(ev->type=='E')
	{
		DSTAT->center_freq_khz += ev->value;
		if(DSTAT->center_freq_khz < 140)
			DSTAT->center_freq_khz = 140;
		if(DSTAT->center_freq_khz > 250)
			DSTAT->center_freq_khz = 250;
		ui_update_view();
	}
	return 0;
}
