#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define TESTCONFIG_PATH		"testconfig.txt"

typedef struct {
	char type;	// L - LFM, P - PSK, F - one freq
	uint16_t fstart_khz;
	uint16_t fstop_khz;
	uint16_t dur_us;
	//full sym length, including tho half-transitions on edges.
	//dur_us = psk_sym_dur*psk_sym_count
	uint8_t psk_sym_dur_us;
	//sym-to-sym transition 
	uint8_t psk_trans_us;
	//PSK data, 1bit=1symbol, MSB-first
	uint16_t psk_pattern;
	uint8_t psk_sym_count;
	uint8_t npings;
} record_cfg_t;

record_cfg_t records[16];
int nrec = 0;

char names[16][16];
int names_num = 0;
int cur_num = 0;
void conf_parse_names()
{
	FILE *fp = fopen(TESTCONFIG_PATH,"r");
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
}

void conf_next_name(int direction)
{
	cur_num = (cur_num+names_num+direction)%names_num;
	printf("Next name: %s\n",names[cur_num]);
}

void get_config(int n, int cfreq)
{
	FILE *fp = fopen(TESTCONFIG_PATH,"r");

	nrec = 0;
	char str[256];
	char substr[24];
	char pattern[20];
	char *pp;
	int inside = 0;

	int freq0,freq1;
	char absrel;
	int abs;
	record_cfg_t *r;
	while(fgets(str,256,fp))
	{
		if(*str=='#' || *str==0)
			continue;
		if(!inside)
		{
			sprintf(substr, "test %s",names[n]);
			char *ptr = strstr(str, substr);
			if(ptr == str)
				inside = 1;
			continue;
		}

		r = &records[nrec];
		nrec++;
		memset(r,0,sizeof(record_cfg_t));

		if(*str == 'L')
		{
			sscanf(str, "L %c %d %d %d %d",
				&absrel,
				&r->npings,
				&freq0,
				&freq1,
				&r->dur_us
			);
			
			if(absrel == 'R')
			{
				freq0 += cfreq;
				freq1 += cfreq;
			}
			r->type = 'L';
			r->fstart_khz = freq0;
			r->fstop_khz = freq1;
		}
		else if(*str == 'F')
		{
			sscanf(str, "F %c %d %d %d",
				&absrel,
				&r->npings,
				&freq0,
				&r->dur_us
			);
			
			if(absrel == 'R')
			{
				freq0 += cfreq;
			}
			r->type = 'F';
			r->fstart_khz = freq0;
		}
		else if(*str == 'P')
		{
			sscanf(str, "P %c %d %d %d %d %d %s",
				&absrel,
				&r->npings,
				&freq0,
				&r->psk_sym_dur_us,
				&r->psk_trans_us,
				&r->psk_sym_count,
				pattern
			);
			
			printf("NP: %d\n",r->npings);

			if(absrel == 'R')
			{
				freq0 += cfreq;
			}
			r->type = 'P';
			r->fstart_khz = freq0;
			r->psk_pattern = 0;

			for(int i=0; i < 16; i++)
			{
				if(!pattern[i])break;
				if(pattern[i]=='1')
					r->psk_pattern |= 1 << (15-i);
			}
			
		}
		
	}

	fclose(fp);
}


int main()
{
	
	conf_parse_names();

	get_config(2,200);


	for(int i=0; i < nrec; i++)
	{
		printf(
			"%c %d %d %d %d %d %d %d %x\n",
			records[i].type,
			records[i].npings, 
			records[i].fstart_khz,
			records[i].fstop_khz, 
			records[i].dur_us,
			records[i].psk_sym_dur_us,
			records[i].psk_trans_us,
			records[i].psk_sym_count,
			records[i].psk_pattern
		);
	}

	
	return 0;
}