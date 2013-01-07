#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stdio.h>

#include "serial.h"
#include "ht1632c.h"


static const PROGMEM char hello_str[]="\n\n"
	"\t****************************************************\n"
	"\t**  JY-MCU 3208 Lattice Clock                     **\n"
	"\t**  (c) 2013 Christian Vogel <vogelchr@vogel.cx>  **\n"
	"\t****************************************************\n\n";

static uint8_t bright = 0;
static uint8_t addr = 0;

static uint8_t fb[32];


static void
zigzag(int8_t *direction, uint8_t *ctr, uint8_t max)
{
	if(!*direction)
		return;

	if(*direction > 0){
		if(*ctr < max)
			(*ctr)++;
		else
			*direction = -1;
	} else {
		if(*ctr > 0)
			(*ctr)--;
		else
			*direction = 1;
	}
}

int8_t demo;
uint8_t ctr;

int
main(void)
{
	char c;

	serial_init();
	ht1632c_init();

	puts_P(hello_str);

	for(c=0;c<32;c++)
		fb[(int)c]=c;
	ht1632c_flush_fb(fb);

	demo=1;

	for(;;){
		if(demo){
			zigzag(&demo,&ctr,31);
			if(demo > 0)
				fb[ctr]=0;
			else
				fb[ctr]=0xff;
			ht1632c_flush_fb(fb);
		}

		if(! serial_status())
			continue;
		c = getchar();

		if(c == 'd')
			demo=1;
		if(c == 'D')
			demo=0;

		if(c == '-' || c == '+'){
			if(c == '+' && bright < 15)
				bright++;
			if(c == '-' && bright > 0)
				bright--;
			ht1632c_bright(bright);
			printf_P(PSTR("Brightness set to %d.\n"),bright);
		}
		if(c == 'o' || c == 'f')
			ht1632c_onoff(c == 'o');
		if(c == 'B' || c == 'b')
			ht1632c_blinkonoff(c == 'B');
	}
}
