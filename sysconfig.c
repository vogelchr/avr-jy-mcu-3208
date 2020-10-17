#include "sysconfig.h"

#include <avr/pgmspace.h>

struct sysconfig sysconfig;

static const struct sysconfig PROGMEM default_sysconfig = {
	.brightness = 2,
	.timer_set = { .u32 = 0x01000000 },
};

void sysconfig_init(void)
{
	memcpy_P(&sysconfig, &default_sysconfig, sizeof(struct sysconfig));
}