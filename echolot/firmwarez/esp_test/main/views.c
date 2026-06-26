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

void view_main_update()
{
	printf("View main update\n");
	char str[20];
	lcd_set_origin(0,0);
	lcd_text_format(2,COLOR_WHITE,COLOR_DARKGREEN,1);
	lcd_draw_string("NXT FIL");	
	lcd_stack_right(4,0);
	lcdc->text_bg = 0;
	sprintf(str,"%d",DSTAT->next_filenum);
	lcd_draw_string(str);
	lcd_stack_down(0,10);
	lcdc->x = 10;
	sprintf(
		str,
		DSTAT->depth_set_mm >= 10000 ? "%.0f" : "%.1f",
		DSTAT->depth_set_mm/1000.0
	);
	lcd_text_format(4,COLOR(0xCCCCCC),COLOR(0x000055),6);
	lcd_draw_string(str);
	// lcdc->text_bg = 0;
	lcd_stack_right(0,0);	
	lcd_draw_string("m");
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

void view_osc_update()
{
	printf("View osc update\n");
	lcd_color_test();

	printf("Reading...");
	dsp_read_wav("/sdcard/test.wav",12500,0);
	printf("Read OK!\n");
	lcd_draw_osc(10000);
	lcd_redraw();
}

int view_osc_on_event(event_t *ev)
{
	ui_print_event("View osc received event",ev);
	return 0;
}