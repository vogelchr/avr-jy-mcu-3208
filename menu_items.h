#ifndef MENU_ITEM_H
#define MENU_ITEM_H

#include <stdint.h>

#define MENU_CONTEXT_CHANNEL                                                   \
	1 /* ==arg2  then arg3 is the channel to be edited later */

struct menu_item {
	const char *name; /* 2 */
	uint8_t next_id; /* 3 */
	uint8_t prev_id; /* 4 */
	uint8_t action; /* 5 */
	uint8_t arg1; /* 6 */
	uint8_t arg2; /* 7 */
	uint8_t arg3; /* 8 */
};

enum chooser_item_type {
	CHOOSER_ITEM_OUTPUT_MODE,
	CHOOSER_ITEM_OUTPUT_POLARITY,
	CHOOSER_ITEM_INPUT_POLARITY,
	CHOOSER_ITEM_TIMER_DIRECTION,
};

struct chooser_item {
	const char *name;
	uint8_t next_id;
	uint8_t prev_id;
	uint8_t value;
};

extern const PROGMEM struct menu_item menu_items[];
extern const PROGMEM struct chooser_item chooser_items[];

#endif