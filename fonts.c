#include "fonts.h"

static PROGMEM const struct font_info * const fonts[1] = {
	& font6x8_info
};

void
fonts_put_char(int fontnum,char c,uint8_t *fb,int x, int y){
	const PROGMEM struct font_info *fi = fonts[fontnum];

}
