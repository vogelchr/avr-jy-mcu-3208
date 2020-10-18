#include "sysconfig.h"

#include <avr/pgmspace.h>
#include <avr/eeprom.h>

struct sysconfig sysconfig;

static const struct sysconfig PROGMEM default_sysconfig = {
	.brightness = 2,
	.timer_set = { .u32 = 0x01000000 },
};

#define RECALL_DEFAULT_PIN_MASK (_BV(7) | _BV(6) | _BV(5))

void sysconfig_init(void)
{
	uint16_t delay = 0;

	eeprom_read_block(&sysconfig, 0, sizeof(struct sysconfig));

	DDRD &= ~RECALL_DEFAULT_PIN_MASK;
	PORTD |= RECALL_DEFAULT_PIN_MASK;

	/* short pause, 0x10000 cycles */
	do {
		delay--;
	} while (delay);

	if ((PIND & RECALL_DEFAULT_PIN_MASK) == 0 ||
	    sysconfig.magic != SYSCONFIG_MAGIC) {
		memcpy_P(&sysconfig, &default_sysconfig,
			 sizeof(struct sysconfig));
		eeprom_write_block(&sysconfig, 0, sizeof(struct sysconfig));
	}
}