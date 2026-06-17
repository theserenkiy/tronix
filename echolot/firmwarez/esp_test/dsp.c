#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>

uint16_t average(uint16_t* buf, size_t len, double initial)
{
	double avg = (double)initial;
	for(int i=0; i < len; i++)
	{
		// printf("%d\n",buf[i]);
		avg += (buf[i] - avg)/(i+1);
	}
	return (uint16_t)avg;
}

void sub_dc(uint16_t* buf, int16_t* res, size_t len, uint16_t dc)
{
	for(int i=0; i < len; i++)
	{
		res[i] = buf[i]-dc;
	}
}

void write_file(char* name, char* buf, size_t size)
{
	FILE* fp = fopen(name, "w");
	int offs = 0, written;
	while(1)
	{
		written = fwrite(buf+offs, 1, size > 1024 ? 1024 : size, fp);
		// printf("Written: %d\n",written);
		size -= written;
		offs += written;
		if(size <= 0)
			break;
	}
	fclose(fp);
}

int main()
{
	FILE* fp = fopen("extracted.bin","r");

	fseek(fp,0,SEEK_END);
	int fsize = ftell(fp);

	if(fsize%2)
		fsize--;
	
	if(fsize <= 0)
	{
		printf("Zero length file. Abort.\n");
		return -1;
	}

	printf("File size: %d\n",fsize);
	fseek(fp, 0, SEEK_SET);

	uint16_t* buf = (uint16_t*)malloc(fsize);

	if(!buf)
	{
		printf("Memory allocation failed\n");
		return -1;
	}

	int samples = fsize >> 1;

	size_t read_samples;
	int offset = 0;
	while(!feof(fp))
	{
		read_samples = fread(buf+offset,2,1024,fp);
		printf("Samples read: %d\n",read_samples);
		offset += read_samples;
	}
	fclose(fp);

	for(int i=0; i < 10; i++)
	{
		printf("%d: %d\n", i, buf[i]);
	}

	int half = samples >> 1;

	uint16_t dc = average(buf+half,half,2048);

	printf("dc: %d\n",dc);

	int16_t* sig = (int16_t*)malloc(samples << 1);

	sub_dc(buf, sig, samples, dc);
	
	write_file("ac.bin", (char*)sig, samples << 1);

	return 0;
}