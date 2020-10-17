#ifndef FONTS_H
#define FONTS_H

#include <stddef.h>
#include <avr/pgmspace.h>

enum font { FONT6X8, FONT4X5 };

enum font_stamp_mode {
	FONT_STAMP_NORM, /* replace character area */
	FONT_STAMP_INV /* replace character area with inverse */
};

struct font_info {
	uint8_t width, height, nchars;
	const uint8_t *data;
	const struct font_offset *offs;
};

struct font_offset {
	uint8_t code;
	uint8_t dummy;
	uint16_t offs;
};

extern uint8_t font_puts_RAM(enum font fontnum, const char *s, uint8_t *fb,
			     int x, int xspace, int y,
			     enum font_stamp_mode mode);

extern uint8_t font_puts_P(enum font fontnum, const char *s, uint8_t *fb, int x,
			   int xspace, int y, enum font_stamp_mode mode);

extern uint8_t fonts_put_char(enum font fontnum, char c, uint8_t *fb, int x,
			      int xspace, int y, enum font_stamp_mode mode);

extern const PROGMEM struct font_info font6x8_info;
extern const PROGMEM struct font_info font4x5_info;

#endif