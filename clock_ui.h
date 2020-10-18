#ifndef CLOCK_UI_H
#define CLOCK_UI_H
#include <stdint.h>

enum clock_ui_state {
	CLOCK_UI_TIME_BIG,
	CLOCK_UI_MENU,
	CLOCK_UI_CHOOSER,
	CLOCK_UI_BRIGHTNESS,
	CLOCK_UI_TIMER_SET,
	CLOCK_UI_TIME_NOCHANGE = 0xff
};

extern uint8_t fb[32]; /* framebuffer */

extern void clock_ui_init();
extern void clock_ui_poll();

#endif