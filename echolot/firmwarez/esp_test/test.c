#include <stdio.h>
#include <inttypes.h>
#include <time.h>

int main()
{
	time_t rawtime;
	time(&rawtime);
	printf("TIME: %d\n",(int)rawtime);
	return 0;
}