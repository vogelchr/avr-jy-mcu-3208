#ifndef HT1632C_H
#define HT1632C_H

#include <stdint.h>

#define HT1632C_WIDTH 32
#define HT1632C_HEIGHT 8

/* set brightness, val = 0 (1/16 pwm) ... 15 (16/16 pwm) */
extern void ht1632c_bright(uint8_t val);

/* onoff: val==0: turn off system oscillator and LED duty cycle generator
 *        val==1: turn on system oscillator */
extern void ht1632c_onoff(uint8_t val);

/* val==0: turn off LED duty cycle generator;
 * val==1: turn on LED duty cycle generator */
extern void ht1632c_ledonoff(uint8_t val);

/* turn on or off blinking function */
extern void ht1632c_blinkonoff(uint8_t val);

/* val == 0: slave mode, 1: master mode */
extern void ht1632c_slave(uint8_t val);

/* 0: on chip RC, 1: external clock */
extern void ht1632c_clock(uint8_t val);

/* options: 0: n-mos open drain output, 8 common option
 *          1: n-mos open drain output, 16 common option
 *          2: p-mos open drain output, 8 common option
 *          3: p-mos open drain output, 16 common option
 */
extern void ht1632c_opts(uint8_t val);

/* set up everything related to the ht1632c */
extern void ht1632c_init(void);

/* write 4 bits to ht1632c data ram */
extern void ht1632c_data4(uint8_t addr, uint8_t nibble);

/* write 4 MSBs of byte to addr, 4 LSB of byte to addr+1 */
extern void ht1632c_data8(uint8_t addr, uint8_t byte);

/* flush a 32byte/8bit framebuffer to LED matrix */
extern void ht1632c_flush_fb(uint8_t *fbmem);

/* clear framebuffer (all LEDs off) */
extern void ht1632c_clear_fb(uint8_t *fbmem);

#endif
