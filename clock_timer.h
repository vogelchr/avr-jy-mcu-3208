#ifndef CLOCK_TIMER_H
#define CLOCK_TIMER_H

#include <stdint.h>

#define CLOCK_TIMER_

/* bits returned by clock_timer_get_key():
   a valid keypress is emitted (VALID bit set)
   if the port input bits are stable, and ...

   (a) once after the debounce interval has passed
   (b) once after the "repeat wait" interval has passed
  and
   (c) afterwards once per the "repeat" interval.

   (b) and (c) will additionally have the LONG bit set.
 */
#define CLOCK_TIMER_KEY_TOP 0x80 /* key connected to PORTD7 */
#define CLOCK_TIMER_KEY_MIDDLE 0x40 /* key connected to PORTD6 */
#define CLOCK_TIMER_KEY_BOTTOM 0x20 /* key connected to PORTD5 */
#define CLOCK_TIMER_KEY_LONG 0x10 /* this is a long keypress */
#define CLOCK_TIMER_KEY_VALID 0x01 /* used internally in clock_timer.c! */

/* bits returned by the clock_timer_get_event() function  */

#define CLOCK_TIMER_EVENT_MASK 0x0f
#define CLOCK_TIMER_EVENT_SLOWTICK 0x01 /* upper nibble is counter */
#define CLOCK_TIMER_EVENT_KEY 0x02 /* upper nibble is keys */

#define CLOCK_TIMER_EVENT_ACTIVE(evt) ((evt) != 0)
#define CLOCK_TIMER_EVENT_IS_SLOWTICK(evt)                                     \
	(((evt)&CLOCK_TIMER_EVENT_MASK) == CLOCK_TIMER_EVENT_SLOWTICK)
#define CLOCK_TIMER_EVENT_IS_KEY(evt)                                          \
	(((evt)&CLOCK_TIMER_EVENT_MASK) == CLOCK_TIMER_EVENT_KEY)

union clock_timer_u32_hmsf {
	/* ordering: f is the LSB of u32, h is MSB */
	struct hmsf {
		uint8_t f; /* fractional seconds */
		uint8_t s; /* seconds */
		uint8_t m; /* minutes */
		uint8_t h; /* hours */
	} hmsf;
	uint32_t u32; /* stored as u32 */
};

/* get current time */
extern void clock_timer_get_walltime(uint32_t *now);
extern uint16_t clock_timer_get_ticks(void);
extern void clock_timer_init(void);
extern uint8_t clock_timer_get_event(void);

extern void clock_timer_set_timer(uint32_t value);
extern uint8_t clock_timer_get_flags(void);
extern void clock_timer_set_flags(uint8_t mask, uint8_t value);
#endif