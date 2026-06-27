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

record_cfg_t records[32];
int nrec = 0;

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
	sprintf(substr, "test %s",names[n]);
	printf("Searching |%s|\n",substr);
	record_cfg_t *r;
	while(fgets(str,256,fp))
	{
		if(*str=='#' || *str==0)
			continue;
		if(!inside)
		{
			char *ptr = strstr(str, substr);
			if(ptr == str)
			{
				printf("FOUND\n");
				inside = 1;
				sprintf(substr, "test ",names[n]);
			}
			continue;
		}

		char *ptr = strstr(str, substr);
		if(ptr == str)
			break;	

		if(*str == 'L' || *str == 'F' || *str == 'P')
		{
			r = records + nrec;
			nrec++;
			memset(r,0,sizeof(record_cfg_t));
		}

		int np;

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
			// printf("F found\n");
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
				&np,
				&freq0,
				&r->psk_sym_dur_us,
				&r->psk_trans_us,
				&r->psk_sym_count,
				pattern
			);
			
			printf("nrec: %d\n",nrec);
			printf("NP: %d\n",np);
			r->npings = np;

			if(absrel == 'R')
			{
				freq0 += cfreq;
			}
			r->type = 'P';
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

	fclose(fp);
}

//type npings fstart fstop dur sym_dur trans_dur sym_cnt pattern
#define WAV_INFO_SZ 2048
char str[WAV_INFO_SZ];
typedef struct {
	char type;
	uint16_t pings;
	uint16_t f0;
	uint16_t f1;
	uint16_t dur;
} cfg_short_t;

cfg_short_t cfgs[64];
cfg_short_t *ping_cfgs[128];
int cfglen = 0;
void parse_header(FILE *fp)
{
	fseek(fp, 0x38, SEEK_SET);
	fread(str,1,WAV_INFO_SZ,fp);
	char *ptr = str;
	str[WAV_INFO_SZ-1]=0;
	// printf("META %s\n",str);

	char typ;
	int f0,f1,dur,sdur,pings;
	cfglen = 0;
	cfg_short_t *c;
	int ret;
	while (ptr = strstr(ptr, "\n"))
	{
		// printf("NL found str=%p ptr=%p char=|%c|\n",str,ptr,*ptr);
		if(ptr >= str+WAV_INFO_SZ-1)
		{
			printf("Break\n");
			break;
		}
		ptr++;
		if((*ptr == 'L' || *ptr == 'F' || *ptr == 'P') && *(ptr+1)==' ')
		{
			ret = sscanf(ptr,"%c %d %d %d %d %d",&typ,&pings,&f0,&f1,&dur,&sdur);
			if(ret && pings)
			{
				c = &cfgs[cfglen];
				c->type = typ;
				c->pings = pings;
				c->f0 = f0;
				c->f1 = f1;
				c->dur = sdur ? sdur : dur;
				cfglen++;
			}
		}
	}
	
	int pci = 0;
	for(int i=0; i < cfglen; i++)
	{
		c = &cfgs[i];
		// printf("%c %d %d %d %d\n",c->type,c->pings,c->f0,c->f1,c->dur);
		for(int j=0; j < c->pings; j++)
		{
			ping_cfgs[pci++] = c;
		}
	}

}


int main()
{
	
	// conf_parse_names();

	// get_config(2,200);

	// printf("%d found\n",nrec);

	// for(int i=0; i < nrec; i++)
	// {
	// 	print_rec(records+i);
	// }

	FILE *fp = fopen("save_000159.wav","rb");

	parse_header(fp);

	fclose(fp);

	return 0;
}