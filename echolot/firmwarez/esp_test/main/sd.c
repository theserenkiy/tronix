#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "driver/spi_master.h"
#include "sd.h"
#include "common.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/gpio.h"
#include "cons.h"
#include "gps.h"
#include "wav.h"

int sd_mounted;

sdmmc_card_t *card;

void sd_init()
{
	printf("Initing SD card...\n");
	sd_mounted = 0;

	sdmmc_host_t host = SDSPI_HOST_DEFAULT();
	host.slot = SPI2_HOST;

	sdspi_device_config_t slot_config =	SDSPI_DEVICE_CONFIG_DEFAULT();

	slot_config.gpio_cs = SD_CS_PIN;
	slot_config.host_id = SPI2_HOST;

	esp_vfs_fat_sdmmc_mount_config_t mount_config = {
		.format_if_mount_failed = false,
		.max_files = 5,
		.allocation_unit_size = 16 * 1024
	};


	esp_err_t ret = esp_vfs_fat_sdspi_mount(
		"/sdcard",
		&host,
		&slot_config,
		&mount_config,
		&card
	);

	if (ret != ESP_OK)
	{
		con_err("SDcard mount failed");
		sd_mounted = 0;
		return;
	}

	printf("SD mount OK\n");
	sdmmc_card_print_info(stdout, card);
	sd_mounted = 1;

	delay_ms(100);


	// if(!sd_check())
	// {
	// 	printf("Trying to write check.tmp...\n");
	// 	FILE *fp = fopen("/sdcard/check.tmp", "w");
	// 	char c = '1';
	// 	fwrite(&c,1,1,fp);
	// 	fclose(fp);
	// 	printf("check.tmp written!\n");
	// }

	

	// FILE *fp = fopen("/sdcard/test.txt","r");
	// char s[256];
	// fread(s,1,256,fp);
	// fclose(fp);
	// printf("FILE CONTENTS: %s\n",s);

}

int sd_check()
{
	printf("Checking SD...\n");
	if(!sd_mounted)
		return 0;
	
	// FILE *fp = fopen("/sdcard/check.tmp", "r");
	// if(!fp)
	// {
	// 	printf("SD check failed...\n");
	// 	return 0;
	// }
	// fclose(fp);
	// printf("SD check OK!\n");

	int ret = sdmmc_get_status(card);
	printf("SD STATUS: %d\n",ret);
	if (ret) {
        printf("Карта не отвечает! Демонтаж файловой системы...\n");
		if(sd_mounted)
		{
        	esp_vfs_fat_sdcard_unmount("/sdcard",card);
			sd_mounted = 0;
		}
        return 0;
    }

	return 1;
}

void getCurBaseFileName(char *bname)
{
	uint16_t num = 1;
	FILE *fp = fopen("/sdcard/_num","r");
	if(fp)
	{
		int len = fscanf(fp, "%hd", &num);
		fclose(fp);
		if(!len)
			num = 1;
	}

	fp = fopen("/sdcard/_num","w");
	fprintf(fp, "%hd", num+1);
	fclose(fp);

	sprintf(bname, "/sdcard/save_%06d", num);
}

int sd_save_ping(uint16_t *buf, size_t len)
{
	printf("Saving to SD...\n");
	if(!sd_check())
	{
		con_err("SD card not connected\n");
		return 0;
	}

	char bname[24];
	getCurBaseFileName(bname);

	printf("Bname: %s\n", bname);

	char fname[32];
	sprintf(fname, "%s.wav",bname);

	printf("Trying save to %s\n",fname);

	save_wav(buf, len, fname, 3125, 2);
	

	// sprintf(fname, "%s.info",bname);
	// FILE *fp = fopen(fname,"w");

	// char info[1024];
	// gps_get()
	// fwrite();
	// fclose(fp);

	printf("File write done\n");

	return 1;
}