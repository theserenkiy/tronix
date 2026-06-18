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


static int sd_mounted = 0;

void sd_init()
{
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


	sdmmc_card_t *card;

	esp_err_t ret = esp_vfs_fat_sdspi_mount(
		"/sdcard",
		&host,
		&slot_config,
		&mount_config,
		&card
	);

	if (ret != ESP_OK)
	{
		printf("SD mount failed: %s\n",
			esp_err_to_name(ret)
		);
		sd_mounted = 0;
		return;
	}

	sd_mounted = 1;
	FILE *fp = fopen("check.tmp", "w");
	fclose(fp);

	sdmmc_card_print_info(stdout, card);

	fp = fopen("/sdcard/test.txt","r");
	char s[256];
	fread(s,1,256,fp);
	fclose(fp);
	printf("FILE CONTENTS: %s\n",s);

}

static int sd_check()
{
	if(!sd_mounted)
		return 0;
	
	FILE *fp = fopen("check.tmp", "r");
	if(!fp)
		return 0;
	fclose(fp);
	return 1;
}

void getCurBaseFileName(char *strnum)
{
	FILE *fp = fopen("/sdcard/_num","r");
	int num;
	int len = fscanf(fp, "%d", &num);
	fclose(fp);
	if(!len)
		num = 1;

	fp = fopen("/sdcard/_num","w");
	fprintf(fp, "%d", num+1);
	fclose(fp);

	sprintf(strnum, "/sdcard/save_%06d", num);
}

void sd_save_ping(uint16_t *buf, size_t len)
{
	if(!sd_check())
	{
		con_err("SD card not connected\n");
		return 0;
	}

	char bname[12];
	getCurBaseFileName(bname);

	char fname[20];
	sprintf(fname, "%s.bin",bname);

	FILE *fp = fopen(fname, "w");
	fwrite(buf, 2, len, fp);
	fclose(fp);

	// sprintf(fname, "%s.info",bname);
	// FILE *fp = fopen(fname,"w");

	// char info[1024];
	// gps_get()
	// fwrite();
	// fclose(fp);

	printf("File write done\n");
	
}