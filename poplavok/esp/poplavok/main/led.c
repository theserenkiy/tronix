#include "led.h"


int ledpins[] = {LED_PIN_G, LED_PIN_R};

void led_init()
{
	int pin;
	for(int i=0; i < 2; i++)
	{
		pin = ledpins[i];
		gpio_set_direction(pin, GPIO_MODE_OUTPUT);
		gpio_set_drive_capability(pin, GPIO_DRIVE_CAP_1); 
		gpio_set_level(pin,0);
	}
}

//0b00 - off, 0b10 - red, 0b01 - green
void led_set_levels(uint8_t levels)
{
	if(levels > 2)levels = 0;
	
	gpio_set_level(ledpins[0],!!(levels & 0b10));
	gpio_set_level(ledpins[1],!!(levels & 0b01));
}