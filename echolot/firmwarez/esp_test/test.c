#include <stdio.h>
#include <inttypes.h>
#include <time.h>

#define COLOR_SWAP(ptr_u16) (ptr_u16 = (ptr_u16) >> 8 | (ptr_u16) << 8)

#define COLOR(C)(((C >> 16) & 0xF8)  | ((C << 3) & 0xE000) | ((C >> 13) & 0x7) | ((C << 5) & 0x1F00))

// static inline color_swap(uint16_t *color)
// {
// 	*color = (*color) >> 8 | (*color) << 8;
// }

int main()
{
	// uint16_t buf[2] = {0x0ff0,0x0880};
	// for(int i=0; i < 2; i++)
	// {
	// 	color_swap(buf+i);
	// }
	// uint8_t *buf8 = (uint8_t *)buf;
	
	// printf("%x %x   %x %x\n",buf8[0],buf8[1],buf8[2],buf8[3]);

	uint16_t c = COLOR(0xFF2080);
	uint8_t *c8 = (uint8_t*)&c;

	printf("%x %x %x\n",c,c8[0],c8[1]);

	return 0;
}