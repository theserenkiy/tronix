#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lib.h"
#include "common.h"

inline void delay_ms(int ms)
{
	vTaskDelay(pdMS_TO_TICKS(ms));
}


inline void delay_us(int us)
{
	esp_rom_delay_us(us);
}

void dump_saves()
{
	if(!DSTAT->sd_ok)
		return;

	FILE *fp = fopen("/sdcard/_save.bin","wb");
	int data[2];
	data[0] = DSTAT->center_freq_khz;
	data[1] = DSTAT->testcfg_idx;
	fopen("_save.bin","wb");
	fwrite(data,sizeof(int),2,fp);
	fclose(fp);
}

void load_saves()
{
	if(!DSTAT->sd_ok)
		return;

	FILE *fp = fopen("/sdcard/_save.bin","rb");
	if(!fp)
	{
		dump_saves();
		return;
	}
	int data[2];
	fread(data,sizeof(int),2,fp);
	fclose(fp);
	DSTAT->center_freq_khz = data[0];
	DSTAT->testcfg_idx = data[1];
}