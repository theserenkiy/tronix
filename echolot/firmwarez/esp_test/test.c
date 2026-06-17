#include <stdio.h>
#include <inttypes.h>

int main()
{
	uint16_t a = 0b0101010111110000;
	uint8_t* ap = (uint8_t*)&a;
	printf("A: %x %x\n",ap[0],ap[1]);
	return 0;
}