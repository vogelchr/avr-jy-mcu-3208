#ifndef SYSCONFIG_H
#define SYSCONFIG_H

#include "clock_timer.h"
#include <stdint.h>

#define NUM_OP_CHANNELS 3
#define NUM_IP_CHANNELS 2

#define SYSCONFIG_TIMER_FLAGS_RUNNING 0x01
#define SYSCONFIG_TIMER_FLAGS_DIR_DOWN 0x02

struct sysconfig {
	uint8_t brightness;
	uint8_t timer_flags;
	uint8_t output_mode[NUM_OP_CHANNELS];
	uint8_t output_polarity[NUM_OP_CHANNELS];
	uint8_t input_polarity[NUM_IP_CHANNELS];
	union clock_timer_u32_hmsf timer_set;
};

extern struct sysconfig sysconfig;

extern void sysconfig_init(void);

#endif