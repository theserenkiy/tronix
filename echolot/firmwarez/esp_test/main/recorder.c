#include "common.h"
#include "sonar.h"
#include "gen.h"
#include "wav.h"
#include "recorder.h"

record_cfg_t testcfg_records[TESTCFG_MAX_ITEMS];
uint8_t testcfg_records_len;

void recorder_init()
{
	// chirp_init();
}

void recorder_test()
{
	gen_chirp(215000, 215000, 1000e-6);
	// gen_psk(100000,100e-6,20e-6,0b1001010100000000,8);
	while (1)
	{
		sonar_ping(NULL);
		
	}
}

static void flush_buf(FILE *fp, int len)
{
	printf("Flush buf\n");
	if(!RECORDER_NO_WRITE)
		fwrite(sonar_buffer,2,len,fp);
	printf("Flush OK!\n");
}

void recorder_make_file_path(char *path)
{
	DSTAT->last_filenum++;
	sprintf(
		(char *)DSTAT->last_filename,
		"save_%06d_%s.wav",
		DSTAT->last_filenum, 
		confparser_get_name()
	);
	sprintf(path, "/sdcard/%s", DSTAT->last_filename);
}


void recorder_record_by_config()
{
	record_cfg_t *cnf = testcfg_records;
	int len = testcfg_records_len;
	int total_sz = 0;
	char text[WAV_INFO_SZ];
	int ping_num = 0;
	memset(text,(int)' ',WAV_INFO_SZ);
	// uint16_t txtpos = sprintf(text, "{\"depth\": %.1f,\n\"records\": [",DSTAT->depth);
	uint16_t txtpos = sprintf(
		text, 
		"depth %.1f\nping_samples %d\nfs %d\nreal_fs %d\ntype npings fstart fstop dur sym_dur trans_dur sym_cnt pattern\nrecords\n",
		DSTAT->depth_set_mm/1000.0, 
		ADC_RECORD_SAMPLES,
		ADC_SAMPLE_FREQ_HZ,
		ADC_REAL_FS_HZ
	);
	for(int i=0; i < len; i++)
	{
		txtpos += snprintf(
			text+txtpos,
			WAV_INFO_SZ-txtpos,
			"%c %d %d %d %d %d %d %d %x\n",
			cnf[i].type,
			cnf[i].npings, 
			cnf[i].fstart_khz,
			cnf[i].fstop_khz, 
			cnf[i].dur_us,
			cnf[i].psk_sym_dur_us,
			cnf[i].psk_trans_us,
			cnf[i].psk_sym_count,
			cnf[i].psk_pattern
		);
		total_sz += cnf[i].npings*ADC_RECORD_SAMPLES;
	}
	// snprintf(text+txtpos,WAV_INFO_SZ-txtpos,"\n]]}\n");
	printf("WAV INFO: %s\n",text);
	// return;
	char path[24];

	FILE *fp = NULL;
	if(!RECORDER_NO_WRITE)
	{
		recorder_make_file_path(path);
		fp = wav_open(path, total_sz, 3125, text);
	}
	
	for(int i=0; i < len; i++)
	{
		if(cnf[i].type=='L')
		{
			gen_chirp(
				cnf[i].fstart_khz*1000,
				cnf[i].fstop_khz*1000, 
				cnf[i].dur_us/1e6
			);
			
		}
		else if(cnf[i].type=='F')
		{
			gen_chirp(
				cnf[i].fstart_khz*1000,
				cnf[i].fstart_khz*1000,
				cnf[i].dur_us/1e6
			);
			// do_ping_series(cnf[i].npings, &ping_num);
		}
		else if(cnf[i].type=='P')
		{
			gen_psk(
				cnf[i].fstart_khz*1000,
				cnf[i].psk_sym_dur_us/1e6,
				cnf[i].psk_trans_us/1e6,
				cnf[i].psk_pattern,
				cnf[i].psk_sym_count
			);
			// do_ping_series(cnf[i].npings, &ping_num);
		}

		for(int p=0; p < cnf[i].npings; p++)
		{
			printf("SONAR PING #%d %dkHz\n",ping_num,cnf[i].fstart_khz);
			sonar_ping(sonar_buffer+((ping_num % SONAR_BUF_SZ_PINGS)*ADC_RECORD_SAMPLES));
			ping_num++;// = (ping_num+1) % SONAR_BUF_SZ_PINGS;
			if(!(ping_num % SONAR_BUF_SZ_PINGS))
				flush_buf(fp,SONAR_BUF_SZ_SAMPLES);
		}
		
	}
	if(ping_num % SONAR_BUF_SZ_PINGS)
		flush_buf(fp, ADC_RECORD_SAMPLES*(ping_num % SONAR_BUF_SZ_PINGS));

	if(!RECORDER_NO_WRITE)
	{
		printf("Closing file...\n");
		fclose(fp);
		printf("OK!\n");
	}
}

void recorder_make_record()
{
	

	recorder_record_by_config();
}

