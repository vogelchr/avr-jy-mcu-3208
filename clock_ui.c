#include "clock_ui.h"
#include "ht1632c.h"
#include "clock_timer.h"
#include "fonts.h"
#include "menu_items.h"
#include "sysconfig.h"

#include <stdio.h>

struct clock_ui_handler {
	void (*on_enter)(uint8_t);
	enum clock_ui_state (*on_refresh)(uint8_t);
};

uint8_t fb[32]; /* framebuffer */

static uint8_t clock_ui_state;
static uint8_t clock_ui_context_ch;

/* ====== CHOOSER ======================================================== */

uint8_t clock_ui_chooser_curr;
enum chooser_item_type clock_ui_chooser_item_type;

/* return the current value for the item identified by clock_ui_chooser_item_type
 and clock_ui_context_ch */
static uint8_t clock_ui_chooser_getval()
{
	uint8_t ch = clock_ui_context_ch;

	switch (clock_ui_chooser_item_type) {
	case CHOOSER_ITEM_OUTPUT_MODE:
		return sysconfig.output_mode[ch];
	case CHOOSER_ITEM_OUTPUT_POLARITY:
		return sysconfig.output_polarity[ch];
	case CHOOSER_ITEM_INPUT_POLARITY:
		return sysconfig.input_polarity[ch];
	case CHOOSER_ITEM_TIMER_DIRECTION:
		return !!(sysconfig.timer_flags &
			  SYSCONFIG_TIMER_FLAGS_DIR_DOWN);
	}
	return 0;
}

/* store the new value for the item identified by clock_ui_chooser_item_type
 and clock_ui_context_ch */

static void clock_ui_chooser_setval(uint8_t v)
{
	uint8_t ch = clock_ui_context_ch;

	switch (clock_ui_chooser_item_type) {
	case CHOOSER_ITEM_OUTPUT_MODE:
		sysconfig.output_mode[ch] = v;
		break;
	case CHOOSER_ITEM_OUTPUT_POLARITY:
		sysconfig.output_polarity[ch] = v;
		break;
	case CHOOSER_ITEM_INPUT_POLARITY:
		sysconfig.input_polarity[ch] = v;
		break;
	case CHOOSER_ITEM_TIMER_DIRECTION:
		clock_timer_set_flags(SYSCONFIG_TIMER_FLAGS_DIR_DOWN,
				      v ? SYSCONFIG_TIMER_FLAGS_DIR_DOWN : 0);
		break;
	}
}

/* enter the chooser, try to find the chooser item that matches the current
value for the sysconfig item  getval() == item->value */
static void clock_ui_chooser_enter(uint8_t old_state)
{
	uint8_t curr, first, next;
	uint8_t currval, itemval;

	(void)old_state;

	curr = first = clock_ui_chooser_curr;
	currval = clock_ui_chooser_getval();

	/* find item corresponding to current value */
	while (1) {
		itemval = pgm_read_byte(&chooser_items[curr].value);
		next = pgm_read_byte(&chooser_items[curr].next_id);
		if (currval == itemval || next == first)
			break;
		curr = next;
	}

	clock_ui_chooser_curr = curr;

	ht1632c_clear_fb(fb);
}

static enum clock_ui_state clock_ui_chooser_refresh(uint8_t event)
{
	uint8_t curr, itemval, currval;
	const char *name;

	if (!event)
		return CLOCK_UI_TIME_NOCHANGE;

	curr = clock_ui_chooser_curr;
	if (CLOCK_TIMER_EVENT_IS_KEY(event)) {
		/* enter a submenu */
		if (event & CLOCK_TIMER_KEY_MIDDLE) {
			uint8_t itemval =
				pgm_read_byte(&chooser_items[curr].value);
			clock_ui_chooser_setval(itemval);
			return CLOCK_UI_MENU;
		}
		if (event & CLOCK_TIMER_KEY_TOP)
			curr = pgm_read_byte(&chooser_items[curr].next_id);
		else if (event & CLOCK_TIMER_KEY_BOTTOM)
			curr = pgm_read_byte(&chooser_items[curr].prev_id);
		clock_ui_chooser_curr = curr;
	}

	currval = clock_ui_chooser_getval();
	itemval = pgm_read_byte(&chooser_items[curr].value);

	ht1632c_clear_fb(fb);

	name = pgm_read_ptr(&chooser_items[curr].name);
	font_puts_P(FONT4X5, name, fb, 2, HT1632C_WIDTH - 4, 1,
		    FONT_STAMP_NORM);

	if (itemval == currval)
		fb[0] = 0x7e;
	else
		fb[0] = 0x00;

	fb[31] = curr;

	ht1632c_flush_fb(fb);

	return CLOCK_UI_TIME_NOCHANGE;
}

/* ====== BRIGHTNESS ======================================================== */

static void clock_ui_brightness_enter(uint8_t old_state)
{
	(void)old_state;
	ht1632c_clear_fb(fb);
}

static enum clock_ui_state clock_ui_brightness_refresh(uint8_t event)
{
	char msg[16];
	uint8_t brightness;

	if (!event)
		return CLOCK_UI_TIME_NOCHANGE;

	brightness = sysconfig.brightness;

	if (CLOCK_TIMER_EVENT_IS_KEY(event)) {
		if (event & CLOCK_TIMER_KEY_MIDDLE) {
			return CLOCK_UI_MENU; /* back */
		}
		if (event & CLOCK_TIMER_KEY_TOP) {
			if (brightness < 15)
				brightness++;
		}
		if (event & CLOCK_TIMER_KEY_BOTTOM) {
			if (brightness > 1)
				brightness--;
		}
		ht1632c_bright(brightness);
		sysconfig.brightness = brightness;
	}

	sprintf_P(msg, PSTR("BRG %2d"), brightness);
	font_puts_RAM(FONT4X5, msg, fb, 0, HT1632C_WIDTH, 0, FONT_STAMP_NORM);
	ht1632c_flush_fb(fb);

	return CLOCK_UI_TIME_NOCHANGE;
}

/* ====== MENU =========================================================== */

static uint8_t clock_ui_menu_curr;

static void clock_ui_menu_enter(uint8_t old_state)
{
	const char *name;
	uint8_t curr;

	(void)old_state;

	curr = clock_ui_menu_curr;
	name = pgm_read_ptr(&menu_items[curr].name);

	ht1632c_clear_fb(fb);
	font_puts_P(FONT4X5, name, fb, 0, HT1632C_WIDTH, 0, FONT_STAMP_NORM);

	ht1632c_flush_fb(fb);
}

static enum clock_ui_state clock_ui_menu_refresh(uint8_t event)
{
	uint8_t curr = clock_ui_menu_curr;
	uint8_t next, action, arg1, arg2, arg3;

	if (CLOCK_TIMER_EVENT_IS_KEY(event)) {
		next = curr;
		action = CLOCK_UI_MENU;

		/* enter a submenu */
		if (event & CLOCK_TIMER_KEY_MIDDLE) {
			action = pgm_read_byte(&menu_items[curr].action);
			arg1 = pgm_read_byte(&menu_items[curr].arg1);
			arg2 = pgm_read_byte(&menu_items[curr].arg2);
			arg3 = pgm_read_byte(&menu_items[curr].arg3);

			/* submenu */
			if (action == CLOCK_UI_MENU) {
				next = arg1;
				if (arg2 == MENU_CONTEXT_CHANNEL)
					clock_ui_context_ch = arg3;
			}

			/* a chooser */
			if (action == CLOCK_UI_CHOOSER) {
				clock_ui_chooser_curr = arg1;
				clock_ui_chooser_item_type = arg2;
			}
		}

		/* prev id */
		if (event & CLOCK_TIMER_KEY_BOTTOM) {
			next = pgm_read_byte(&menu_items[curr].prev_id);
		}

		/* next id */
		if (event & CLOCK_TIMER_KEY_TOP) {
			next = pgm_read_byte(&menu_items[curr].next_id);
		}
		clock_ui_menu_curr = next;
		return action;
	}

	return CLOCK_UI_TIME_NOCHANGE;
}

/* ====== CLOCK ========================================================== */

enum clock_ui_time_big_type {
	CLOCK_UI_TIME_BIG_HM,
	CLOCK_UI_TIME_BIG_MS,
	CLOCK_UI_TIME_BIG_HMS
};
static enum clock_ui_time_big_type clock_ui_time_big_type;
static uint8_t clock_ui_time_big_lastsec;

static void clock_ui_time_big_enter(uint8_t old_state)
{
	(void)old_state;
	clock_ui_time_big_lastsec = 0xff;
	ht1632c_clear_fb(fb);
}

static enum clock_ui_state clock_ui_time_big_refresh(uint8_t event)
{
	union clock_timer_u32_hmsf now;
	char msg[8];
	uint8_t refresh, clocktype, dig1, dig2, colonpix, flags;

	flags = clock_timer_get_flags();

	clocktype = clock_ui_time_big_type;
	/* handle keys */
	if (CLOCK_TIMER_EVENT_IS_KEY(event)) {
		if (event & CLOCK_TIMER_KEY_TOP) {
			if (clocktype == CLOCK_UI_TIME_BIG_HMS) {
				clocktype = 0;
			} else {
				clocktype++;
			}
			clock_ui_time_big_type = clocktype;
			return CLOCK_UI_TIME_BIG;
		}
		if (event & CLOCK_TIMER_KEY_MIDDLE) {
			clock_ui_menu_curr = 0;
			return CLOCK_UI_MENU;
		}
		if (event & CLOCK_TIMER_KEY_BOTTOM) {
			/* toggle running flag */
			flags ^= SYSCONFIG_TIMER_FLAGS_RUNNING;
			clock_timer_set_flags(SYSCONFIG_TIMER_FLAGS_RUNNING,
					      flags);
		}
	}

	clock_timer_get_walltime(&now.u32);

	if (CLOCK_TIMER_EVENT_IS_SLOWTICK(event)) {
		if (flags & SYSCONFIG_TIMER_FLAGS_RUNNING) {
			colonpix = (event & 0x20) ? 0xff : 0x00;
		} else {
			colonpix = 0xff;
		}
	}

	/* limit refresh */
	refresh = now.hmsf.s;
	if (refresh == clock_ui_time_big_lastsec && !event)
		return CLOCK_UI_TIME_NOCHANGE;
	clock_ui_time_big_lastsec = refresh;

	/* draw clocks */
	switch (clocktype) {
	case CLOCK_UI_TIME_BIG_HM: /* HH:MM */
		dig1 = now.hmsf.h;
		dig2 = now.hmsf.m;
		goto draw_digits;
	case CLOCK_UI_TIME_BIG_MS: /* MM:SS */
		dig1 = now.hmsf.m;
		dig2 = now.hmsf.s;
	draw_digits:
		sprintf_P(msg, PSTR("%02d"), dig1);
		font_puts_RAM(FONT6X8, msg, fb, 1, HT1632C_WIDTH - 1, 0,
			      FONT_STAMP_NORM);
		sprintf_P(msg, PSTR("%02d"), dig2);
		font_puts_RAM(FONT6X8, msg, fb, 18, HT1632C_WIDTH - 18, 0,
			      FONT_STAMP_NORM);

		if (CLOCK_TIMER_EVENT_IS_SLOWTICK(event)) {
			colonpix &= 0x66;
			/* blink colon */
			fb[15] = fb[16] = colonpix;
		}

		break;
	default:
	case CLOCK_UI_TIME_BIG_HMS: /* HH:MM:SS */
		sprintf_P(msg, PSTR("%02d"), now.hmsf.h);
		font_puts_RAM(FONT4X5, msg, fb, 0, HT1632C_WIDTH, 3,
			      FONT_STAMP_NORM);

		sprintf_P(msg, PSTR("%02d"), now.hmsf.m);
		font_puts_RAM(FONT4X5, msg, fb, 10, HT1632C_WIDTH - 11, 0,
			      FONT_STAMP_NORM);

		sprintf_P(msg, PSTR("%02d"), now.hmsf.s);
		font_puts_RAM(FONT4X5, msg, fb, 23, HT1632C_WIDTH - 23, 0,
			      FONT_STAMP_NORM);

		if (CLOCK_TIMER_EVENT_IS_SLOWTICK(event)) {
			colonpix &= 0x0a;
			fb[20] = fb[21] = colonpix;
		}
		break;
	} // switch

	ht1632c_flush_fb(fb);
	return CLOCK_UI_TIME_NOCHANGE;
}

PROGMEM static const struct clock_ui_handler clock_ui_handlers[] = {
	[CLOCK_UI_TIME_BIG] = { clock_ui_time_big_enter,
				clock_ui_time_big_refresh },
	[CLOCK_UI_MENU] = { clock_ui_menu_enter, clock_ui_menu_refresh },
	[CLOCK_UI_CHOOSER] = { clock_ui_chooser_enter,
			       clock_ui_chooser_refresh },
	[CLOCK_UI_BRIGHTNESS] = { clock_ui_brightness_enter,
				  clock_ui_brightness_refresh },

};

#define CLOCK_UI_BUTTON_CTR_MAX 10

void clock_ui_poll()
{
	enum clock_ui_state next, curr;
	uint8_t event = 0;

	void (*on_enter)(uint8_t);
	enum clock_ui_state (*on_refresh)(uint8_t);

	event = clock_timer_get_event();
	curr = clock_ui_state;
	on_refresh = pgm_read_ptr(&clock_ui_handlers[curr].on_refresh);

	next = on_refresh(event);
	/* note: also allows changing to current, forcing
	   reinitialization */
	if (next != CLOCK_UI_TIME_NOCHANGE) {
		on_enter = pgm_read_ptr(&clock_ui_handlers[next].on_enter);
		on_enter(curr);
		curr = next;
	}
	clock_ui_state = curr;
}

void clock_ui_init()
{
	clock_ui_state = CLOCK_UI_TIME_BIG;
	clock_ui_time_big_enter(CLOCK_UI_TIME_BIG); /* clear fb */
}
