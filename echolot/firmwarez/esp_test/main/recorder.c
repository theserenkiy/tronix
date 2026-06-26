#include "common.h"
#include "sonar.h"
#include "gen.h"
#include "wav.h"
#include "recorder.h"


void recorder_init()
{
	// chirp_init();
}

void recorder_test()
{
	gen_chirp(178000, 178000, 1000e-6);
	// gen_psk(100000,100e-6,20e-6,0b1001010100000000,8);
	while (1)
	{
		sonar_ping(NULL);
		delay_us(1000);
		sonar_ping(NULL);
		delay_us(1000);
		sonar_ping(NULL);
		delay_us(1000);
		sonar_ping(NULL);
		delay_us(1000);
		delay_ms(1);
	}
}

static void flush_buf(FILE *fp, int len)
{
	fwrite(sonar_buffer,2,len,fp);
}

void recorder_make_file_path(char *path)
{
	sprintf(path, "/sdcard/save_%06d.wav", DSTAT->next_filenum);
	DSTAT->next_filenum++;
}

void recorder_record_by_config(record_cfg_t *cnf, int len)
{
	int total_sz = 0;
	char text[WAV_INFO_SZ];
	int ping_num = 0;
	memset(text,(int)' ',WAV_INFO_SZ);
	// uint16_t txtpos = sprintf(text, "{\"depth\": %.1f,\n\"records\": [",DSTAT->depth);
	uint16_t txtpos = sprintf(
		text, 
		"depth %.1f\nping_samples %d\nrecords\ntype npings fstart fstop dur sym_dur trans_dur sym_cnt pattern\n",
		DSTAT->depth_set_mm/1000.0, 
		ADC_RECORD_SAMPLES
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
	recorder_make_file_path(path);
	FILE *fp = wav_open(path, total_sz, 3125, text);

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
		else
		{
			gen_psk(
				cnf[i].fstart_khz*1000,
				cnf[i].psk_sym_dur_us/1e6,
				cnf[i].psk_trans_us/1e6,
				cnf[i].psk_pattern,
				cnf[i].psk_sym_count
			);
		}
		

		for(int i=0; i < cnf[i].npings; i++)
		{
			sonar_ping(sonar_buffer+(ping_num*ADC_RECORD_SAMPLES));
			ping_num = (ping_num+1) % SONAR_BUF_SZ_PINGS;
			if(!ping_num)
				flush_buf(fp,SONAR_BUF_SZ_SAMPLES);
		}
	}
	if(ping_num)
		flush_buf(fp, ADC_RECORD_SAMPLES*ping_num);
	fclose(fp);
}

void recorder_make_record()
{
	// record_cfg_t rec[] = {
	// 	{
	// 		.type = 'L',
	// 		.fstart_khz = 160,
	// 		.fstop_khz = 200,
	// 		.dur_us = 1000,
	// 		.npings = 2
	// 	},
	// 	{
	// 		.type = 'L',
	// 		.fstart_khz = 185,
	// 		.fstop_khz = 185,
	// 		.dur_us = 400,
	// 		.npings = 2
	// 	}
	// };

	int nfreq = 60;
	int startfreq = 190;
	record_cfg_t rec[60];
	
	for(int i=0; i < nfreq; i++)
	{
		memset(&rec[i],0,sizeof(record_cfg_t));
		rec[i].type = 'L';
		rec[i].fstart_khz = startfreq+i;
		rec[i].fstop_khz = startfreq+i;
		rec[i].dur_us = 1000;
		rec[i].npings = 1;
	}

	recorder_record_by_config(rec,nfreq);
}


// void make_measure()
// {
// 	FILE *fp = sd_open_wav(ADC_RECORD_SAMPLES*NCYCLES*6);

// 	// lcd_sleep(1);
// 	// lcd_sleep(1);
// 	gps_enable(0);
// 	sonar_precharge(500);
// 	for(int cyc=0; cyc < 1; cyc++)
// 	{
// 		for(int i=0; i < 3; i++)
// 		{
// 			// sonar_tx_burst(32, 1);
// 			sonar_ping();
// 			sonar_adc_capture(big_buffer+(ADC_RECORD_SAMPLES *i), ADC_RECORD_SAMPLES);
// 			delay_ms(BURST_TO_BURST_DELAY_MS);
// 			// uart_logger_send_buffer(buffer, ADC_RECORD_SAMPLES);
// 		}

// 		for(int i=3; i < 6; i++)
// 		{
// 			// sonar_tx_burst(32, 1);
// 			sonar_ping2();
// 			sonar_adc_capture(big_buffer+(ADC_RECORD_SAMPLES *i), ADC_RECORD_SAMPLES);
// 			delay_ms(BURST_TO_BURST_DELAY_MS);
// 			// uart_logger_send_buffer(buffer, ADC_RECORD_SAMPLES);
// 		}
// 		fwrite(big_buffer,2,ADC_RECORD_SAMPLES*6,fp);
// 	}
// 	fclose(fp);
// 	printf("File closed\n");
// 	// sd_save_ping(big_buffer, ADC_RECORD_SAMPLES*6);
// 	// lcd_sleep(0);
// 	// lcd_waveform(big_buffer, ADC_RECORD_SAMPLES, 0);
// 	sonar_charge(1);
// 	// gps_enable(1);
// }