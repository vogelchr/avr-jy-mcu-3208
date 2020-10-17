#include "clock_timer.h"
#include "sysconfig.h"

#include <stdio.h>
#include <string.h>
#include <avr/interrupt.h>
#include <util/atomic.h>

#define CLOCK_TIMER_KEY_DEBOUNCE_CYCLES 5 /* 5 * 10ms ==> 50ms */
#define CLOCK_TIMER_KEY_REPEAT_WAIT 125 /* 125 * 10ms ==> 1.25s */
#define CLOCK_TIMER_KEY_REPEAT_CYCLES 25 /* 25 * 10ms ==> 250ms */

#define CLOCK_TIMER_KEY_PORT PORTD
#define CLOCK_TIMER_KEY_DDR DDRD
#define CLOCK_TIMER_KEY_PIN PIND
#define CLOCK_TIMER_KEY_MASK 0xe0 /* 5, 6, 7 */
#define CLOCK_TIMER_KEY_SHIFT 5

static uint32_t clock_timer_wallclock = 0x01020304;
static uint16_t clock_timer_ticks;
static uint8_t clock_timer_event;

static uint8_t clock_timer_key_ctr;
static uint8_t clock_timer_key_prev;
static uint8_t clock_timer_key_state;

ISR(TIMER1_CAPT_vect)
{
	uint8_t timer_flags;
	uint32_t tick;
	union clock_timer_u32_hmsf u, s;
	uint8_t key, key_prev, key_ctr, key_state;

	timer_flags = sysconfig.timer_flags;

	/* wallclock updates */
	if (timer_flags & SYSCONFIG_TIMER_FLAGS_RUNNING) {
		u.u32 = clock_timer_wallclock;
		s = sysconfig.timer_set;

		if (!(timer_flags & SYSCONFIG_TIMER_FLAGS_DIR_DOWN)) {
			/* carry chain up... */
			u.hmsf.f++;
			if (u.hmsf.f >= 100) {
				u.hmsf.s++;
				u.hmsf.f = 0;
				if (u.hmsf.s >= 60) {
					u.hmsf.s = 0;
					u.hmsf.m++;
					if (u.hmsf.m >= 60) {
						u.hmsf.m = 0;
						u.hmsf.h++;
						if (u.hmsf.h >= 24)
							u.hmsf.h = 0;
					}
				}
			}
			if (u.u32 == s.u32)
				timer_flags &= ~SYSCONFIG_TIMER_FLAGS_RUNNING;
		} else {
			/* carry chain down... */
			if (u.hmsf.f == 0) {
				u.hmsf.f = 99;
				if (u.hmsf.s == 0) {
					u.hmsf.s = 59;
					if (u.hmsf.m == 0) {
						u.hmsf.m = 59;
						if (u.hmsf.h == 0) {
							u.hmsf.h = 23;
						} else
							u.hmsf.h--;
					} else
						u.hmsf.m--;
				} else
					u.hmsf.s--;
			} else
				u.hmsf.f--;

			if (u.u32 == 0)
				timer_flags &= ~SYSCONFIG_TIMER_FLAGS_RUNNING;
		}
		clock_timer_wallclock = u.u32;
		sysconfig.timer_flags = timer_flags;
	}

	/* ticks update */
	tick = clock_timer_ticks;
	tick++;

	/* emit slowtick events */
	if ((tick & 0x0000000f) == 0) { /* every 160ms */
		uint8_t evt;
		evt = CLOCK_TIMER_EVENT_SLOWTICK;
		evt |= (tick & 0x000000f0);

		clock_timer_event = evt;
	}
	clock_timer_ticks = tick;

	/* keys are low-active! */
	key = ((CLOCK_TIMER_KEY_PIN ^ CLOCK_TIMER_KEY_MASK) &
	       CLOCK_TIMER_KEY_MASK);
	key_ctr = clock_timer_key_ctr;
	key_prev = clock_timer_key_prev;
	key_state = clock_timer_key_state;

	/* inputs are not stable, or not pressed at all */
	if (!key || (key != key_prev)) {
		key_ctr = 0;
		key_prev = key;
		key_state = 0;
		goto key_out;
	}

	/* inputs are stable */
	key_state = key;

	/*
	  counter increases up to debounce interval, then emits a keypress,
	 then counts to repeat wait, emits a keypress,
	 goes back repeat cycles, counts to repeat wait emits a keypress, ...
	 goes back repeat cycles, counts to repeat wait emits a keypress, ...
	    0, 1, 2, 3, 4, 5*ACTIVE*, 6, 7, 8, 9...80*ACTIVE* 65, 66, ..., 80*ACTIVE*
	*/

	key_ctr++;
	if (key_ctr == CLOCK_TIMER_KEY_DEBOUNCE_CYCLES) {
		key_state = CLOCK_TIMER_EVENT_KEY | key;
	}
	if (key_ctr ==
	    CLOCK_TIMER_KEY_DEBOUNCE_CYCLES + CLOCK_TIMER_KEY_REPEAT_WAIT) {
		key_ctr = CLOCK_TIMER_KEY_DEBOUNCE_CYCLES +
			  CLOCK_TIMER_KEY_REPEAT_WAIT -
			  CLOCK_TIMER_KEY_REPEAT_CYCLES;
		key_state = CLOCK_TIMER_EVENT_KEY | CLOCK_TIMER_KEY_LONG | key;
	}

key_out:
	clock_timer_key_state = key_state;
	clock_timer_key_prev = key_prev;
	clock_timer_key_ctr = key_ctr;
}

extern void clock_timer_get_walltime(uint32_t *now)
{
	union clock_timer_u32_hmsf u;

	ATOMIC_BLOCK(ATOMIC_FORCEON)
	{
		u.u32 = clock_timer_wallclock;
		*now = u.u32;
	}
}

extern uint16_t clock_timer_get_ticks()
{
	uint16_t ret;
	ATOMIC_BLOCK(ATOMIC_FORCEON)
	{
		memcpy(&ret, &clock_timer_ticks, sizeof(clock_timer_ticks));
	}
	return ret;
}

/* return either keys or timer events */
extern uint8_t clock_timer_get_event()
{
	uint8_t ret;
	ATOMIC_BLOCK(ATOMIC_FORCEON)
	{
		ret = clock_timer_key_state;
		clock_timer_key_state = 0;
	}
	if (ret)
		return ret;

	ATOMIC_BLOCK(ATOMIC_FORCEON)
	{
		ret = clock_timer_event;
		clock_timer_event = 0;
	}
	return ret;
}

void clock_timer_set_timer(uint32_t setval)
{
	ATOMIC_BLOCK(ATOMIC_FORCEON)
	{
		uint8_t flags = sysconfig.timer_flags;
		uint32_t wallclock;

		sysconfig.timer_set.u32 = setval;
		flags &= ~SYSCONFIG_TIMER_FLAGS_RUNNING;

		if (flags & SYSCONFIG_TIMER_FLAGS_DIR_DOWN)
			wallclock = setval;
		else
			wallclock = 0;

		clock_timer_wallclock = wallclock;
		sysconfig.timer_flags = flags;
	}
}

uint8_t clock_timer_get_flags(void)
{
	return sysconfig.timer_flags;
}

void clock_timer_set_flags(uint8_t mask, uint8_t value)
{
	ATOMIC_BLOCK(ATOMIC_FORCEON)
	{
		uint8_t flags = sysconfig.timer_flags;

		value &= mask;
		flags &= ~mask;
		flags |= value;

		sysconfig.timer_flags = flags;
	}
}

void clock_timer_init()
{
	/* for keys */
	DDRD &= ~0xe0; /* PORTD5, 6, 7 */
	PORTD |= 0xe0; /* inputs, pullup */

	/* 16 bit Timer/Counter 1 in CTC mode:
         * WGM13=1, WGM12/CTC1=1 WGM11/PCM11=0 WGM10/PWM10=0 */
	TCCR1A = 0;
	TCCR1B = _BV(WGM13) | _BV(WGM12) | _BV(CS11) |
		 _BV(CS10); /* CS12/11/10=010: clkIO/8 */
	TIMSK = _BV(TICIE1);
	/* F_CPU = 8'000'000 / 64 / 1250 = 100 Hz overflow */
	ICR1 = 1249;
}