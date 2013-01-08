#ifndef FONTS_H
#define FONTS_H

#include <stddef.h>
#include <avr/pgmspace.h>

struct font_info {
	uint8_t width,height,nchars;
	const uint8_t * data;
	const struct font_offset *offs;
};

struct font_offset {
	uint8_t code;
	uint8_t dummy;
	uint16_t offs;
};

extern const PROGMEM struct font_info font6x8_info;

#endif