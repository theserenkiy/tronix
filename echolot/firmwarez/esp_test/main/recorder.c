#include "common.h"
#include "sonar.h"
#include "chirp.h"
#include "wav.h"
#include "recorder.h"


void recorder_init()
{
	// chirp_init();
}

void recorder_test()
{
	chirp_gen(178000, 190000, 320);
	while (1)
	{
		sonar_ping(sonar_buffer,1);
		delay_ms(50);
	}
	
}

static void flush_buf(FILE *fp)
{
	fwrite(sonar_buffer,2,SONAR_BUF_SZ_SAMPLES,fp);
}

static void record_pings(FILE *fp)
{
	sonar_ping(sonar_buffer,SONAR_BUF_SZ_PINGS);
	flush_buf(fp);
} 

void recorder_make_record()
{
	FILE *fp = wav_open("/sdcard/test.wav", SONAR_BUF_SZ_SAMPLES*4, 3125, "Test info");

	chirp_gen(160000, 200000, 1000);
	record_pings(fp);
	record_pings(fp);

	chirp_gen(185000, 185000, 320);
	record_pings(fp);
	record_pings(fp);
	
	fclose(fp);
}


// void make_measure()
// {
// 	FILE *fp = sd_open_wav(ADC_RECORD_SAMPLES*NCYCLES*6);

// 	// lcd2_sleep(1);
// 	// lcd2_sleep(1);
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
// 	// lcd2_sleep(0);
// 	// lcd2_waveform(big_buffer, ADC_RECORD_SAMPLES, 0);
// 	sonar_charge(1);
// 	// gps_enable(1);
// }