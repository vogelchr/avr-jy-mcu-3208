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
		fb[c]=c;
	ht1632c_flush_fb(fb);

	for(;;){
		if(demo){
			uint8_t col,row;

			if(demo > 0){
				if(ctr < 255)
					ctr++;
				if(ctr == 255)
					demo = -1;
			} else {
				if(ctr > 0)
					ctr--;
				if(ctr == 0)
					demo = 1;
			}

			col = ctr >> 3;
			row = ctr & 0x07;

			if(demo > 0)
				fb[col] &= ~(1<<row);
			else
				fb[col] |=  (1<<row);
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
		}
		if(c == 'o' || c == 'f')
			ht1632c_onoff(c == 'o');
		if(c == 'B' || c == 'b')
			ht1632c_blinkonoff(c == 'B');
		if(c == '<' || c == '>'){
			ht1632c_data4(addr,0);
			if(c == '>' && addr < 63)
				addr++;
			if(c == '<' && addr > 0)
				addr--;
			ht1632c_data8(addr,0x91);
		}
	}
}
