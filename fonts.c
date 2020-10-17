#include "fonts.h"

#include <stdio.h>

static PROGMEM const struct font_info *const fonts[] = { &font6x8_info,
							 &font4x5_info };

#ifdef DEBUG_FONT_DATA_STRUCTURES
#define DEBUG(...) printf_P(__VA_ARGS)
#else
#define DEBUG(...) /* ... */
#endif

uint8_t font_puts_generic(enum font fontnum, const char *s, uint8_t *fb, int x,
			  int xspace, int y, enum font_stamp_mode mode,
			  int in_progmem)
{
	char c;
	uint8_t ret;
	uint8_t sum = 0;

	while (xspace > 0) {
		if (in_progmem)
			c = pgm_read_byte(s++);
		else
			c = *s++;

		if (!c)
			break;
		ret = fonts_put_char(fontnum, c, fb, x, xspace, y, mode);
		sum += ret;
		x += ret;
		xspace -= ret;

		if (xspace > 0) {
			x++;
			xspace--;
			sum++;
		}
	}
	return sum;
}

uint8_t font_puts_RAM(enum font fontnum, const char *s, uint8_t *fb, int x,
		      int xspace, int y, enum font_stamp_mode mode)
{
	return font_puts_generic(fontnum, s, fb, x, xspace, y, mode, 0);
}

uint8_t font_puts_P(enum font fontnum, const char *s, uint8_t *fb, int x,
		    int xspace, int y, enum font_stamp_mode mode)
{
	return font_puts_generic(fontnum, s, fb, x, xspace, y, mode, 1);
}

uint8_t fonts_put_char(enum font fontnum, char c, uint8_t *fb, int x,
		       int xspace, int y, enum font_stamp_mode mode)
{
	const struct font_info *fip;
	const struct font_offset *offs;
	const uint8_t *data;

	uint8_t width, height, code, mask, bits;
	uint16_t offset_in_data;
	int i;

	fip = (const struct font_info *)pgm_read_word(fonts + fontnum);
	offs = (const struct font_offset *)pgm_read_word(&(fip->offs));
	data = (const uint8_t *)pgm_read_word(&(fip->data));

	DEBUG(PSTR("Font %d based on fip=%p, offs=%p, data=%p\n"), fontnum, fip,
	      offs, data);

	width = pgm_read_byte(&(fip->width));
	height = pgm_read_byte(&(fip->height));

	do {
		code = pgm_read_byte(&(offs->code));
		offset_in_data = pgm_read_word(&(offs->offs));
		if (code == c)
			break;
		offs++;
	} while (code != 0xff); /* end */

	if (code == 0xff) {
		DEBUG(PSTR("  Char 0x%02x not found.\n"), c);
		return 0;
	}

	DEBUG(PSTR("  Char 0x%02x, offset info @ %p, offs = 0x%02x\n"), code,
	      offs, offset_in_data);

	/* seek to requested character */
	data += offset_in_data;
	fb += x;
	mask = ((1 << height) - 1) << y;

	for (i = 0; i < width; i++) {
		if (i >= xspace)
			break;

		bits = pgm_read_byte(data);
		bits <<= y;

		if (mode == FONT_STAMP_INV)
			bits = ~bits;

		data++;
		DEBUG(PSTR("Character 0x%02x, row %d, data @ %p = 0x%02x.\n"),
		      c, i, data, bits);
		*fb = (*fb & ~mask) | (bits & mask);
		fb++;
	}

	return i;
}
