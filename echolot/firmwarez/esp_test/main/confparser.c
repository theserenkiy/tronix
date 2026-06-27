#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "common.h"
#include "confparser.h"

char names[16][16];
int names_num = 0;

void print_rec(record_cfg_t *rec)
{
	printf(
			"%c %d %d %d %d %d %d %d %x\n",
			rec->type,
			rec->npings, 
			rec->fstart_khz,
			rec->fstop_khz, 
			rec->dur_us,
			rec->psk_sym_dur_us,
			rec->psk_trans_us,
			rec->psk_sym_count,
			rec->psk_pattern
		);
}


void confparser_init()
{
	FILE *fp = fopen(TESTCONFIG_PATH,"r");
	if(!fp)
	{
		ui_fatal("No testcnf");
	}
	names_num = 0;
	char str[256];
	char name[16];
	while(fgets(str,256,fp))
	{
		if(*str=='#' || *str==0)
			continue;
		char *ptr = strstr(str, "test ");
		if(ptr == str)
		{
			sscanf(ptr, "test %s", name);
			strncpy(names[names_num++],name,16);
			// printf("Name: %s\n",name);
		}
	}
	fclose(fp);
	if(DSTAT->testcfg_idx >= names_num-1)
		DSTAT->testcfg_idx = 0;
	confparser_next(0);
}

void confparser_next(int direction)
{
	DSTAT->testcfg_idx = (DSTAT->testcfg_idx+names_num+direction)%names_num;
	confparser_get_config(DSTAT->testcfg_idx,DSTAT->center_freq_khz);
	for(int i=0; i < testcfg_records_len;i++)
	{
		print_rec(&testcfg_records[i]);
	}
}

char *confparser_get_name()
{
	return names[DSTAT->testcfg_idx];
}

void confparser_get_config(int n, int cfreq)
{
	FILE *fp = fopen(TESTCONFIG_PATH,"r");

	int nrec = 0;
	char str[256];
	char substr[24];
	char pattern[20];
	int inside = 0;

	int freq0,freq1;
	sprintf(substr, "test %s",names[n]);
	// printf("Searching |%s|\n",substr);
	record_cfg_t *r;
	memset(testcfg_records,0,sizeof(record_cfg_t)*TESTCFG_MAX_ITEMS);
	while(fgets(str,256,fp))
	{
		if(*str=='#' || *str==0)
			continue;
		if(!inside)
		{
			char *ptr = strstr(str, substr);
			if(ptr == str)
			{
				// printf("FOUND\n");
				inside = 1;
				sprintf(substr, "test ");
			}
			continue;
		}

		char *ptr = strstr(str, substr);
		if(ptr == str)
			break;	

		
		if(*str == 'L' || *str == 'F' || *str == 'P' || *str == 'S')
		{
			if(nrec >= TESTCFG_MAX_ITEMS)
			{
				printf("MAX items reached\n");
				break;
			}
			r = testcfg_records + nrec;
			nrec++;
			
		
			r->type = *str;

			if(*str == 'L')
			{
				sscanf(str, "%*c %hhd %d %d %hd",
					&r->npings,
					&freq0,
					&freq1,
					&r->dur_us
				);
				
				if(abs(freq0) < 100)
				{
					freq0 += cfreq;
					freq1 += cfreq;
				}
				// r->npings = np;
				
				r->fstart_khz = freq0;
				r->fstop_khz = freq1;
			}
			else if(*str == 'S')
			{
				int np, dur;
				sscanf(str, "%*c %d %d %d %d",
					&np,
					&freq0,
					&freq1,
					&dur
				);
				
				if(abs(freq0) < 100)
				{
					freq0 += cfreq;
				}
				if(!freq1 || freq1 > 50)
					freq1 = 1;

				nrec--;
				for(int i=0; i < np; i++)
				{
					if(nrec >= TESTCFG_MAX_ITEMS)
					{
						printf("MAX items reached\n");
						break;
					}
					r = testcfg_records+nrec;
					r->type = 'F';
					r->npings = 1;
					r->dur_us = dur;
					r->fstart_khz = freq0;
					freq0 += freq1;
					nrec++;
				}
				// r->npings = np;
				
			}
			else if(*str == 'F')
			{
				// printf("F found\n");
				sscanf(str, "%*c %hhd %d %hd",
					&r->npings,
					&freq0,
					&r->dur_us
				);
				
				if(abs(freq0) < 100)
				{
					freq0 += cfreq;
				}
				
				r->fstart_khz = freq0;
				
			}
			else if(*str == 'P')
			{
				sscanf(str, "%*c %hhd %d %hd %hd %hhd %s",
					&r->npings,
					&freq0,
					&r->psk_sym_dur_us,
					&r->psk_trans_us,
					&r->psk_sym_count,
					pattern
				);
				
				// r->npings = np;

				if(abs(freq0) < 100)
				{
					freq0 += cfreq;
				}

				r->fstart_khz = freq0;
				r->psk_pattern = 0;

				for(int i=0; i < 16; i++)
				{
					// printf("I: %d\n",i);
					if(!pattern[i])
						break;
					if(pattern[i]=='1')
						r->psk_pattern |= (uint16_t)(1 << (15-i));
				}
				
			}
		}
		
	}

	fclose(fp);
	testcfg_records_len = nrec;
}

void confparser_test_summary(char *s)
{
	int pings = 0;
	for(int i=0; i < testcfg_records_len; i++)
	{
		pings += testcfg_records[i].npings;
	}
	sprintf(s,"Itms: %d; pings: %d",testcfg_records_len,pings);
}