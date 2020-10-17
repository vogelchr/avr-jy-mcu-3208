#include "ht1632c.h"

#include <avr/io.h>
#include <stdio.h>

/* set this to the port the controller is connected to */
#define HT1632C_PORT PORTB
#define HT1632C_DDR DDRB
#define HT1632C_CS _BV(3)
#define HT1632C_WRCLK _BV(4)
#define HT1632C_DATA _BV(5)

/*
#define BIT_SLEEP do { asm volatile ("nop;\n\tnop;\n\tnop;\n"); } while(0)
*/
#define BIT_SLEEP                                                              \
	do {                                                                   \
	} while (0)

static void ht1632c_start(void)
{
	BIT_SLEEP;
	HT1632C_PORT &= ~HT1632C_CS;
}

static void ht1632c_stop(void)
{
	BIT_SLEEP;
	HT1632C_PORT |= HT1632C_CS;
}

/* clock out bits to HT1632C, start with the bit
 * indicated by mask (MSB). */
static void ht1632c_bits_mask(uint8_t bits, uint8_t mask)
{
	while (mask) { /* 8 bits, until we have shifted out the 1 */
		HT1632C_PORT &= ~HT1632C_WRCLK;
		if (bits & mask)
			HT1632C_PORT |= HT1632C_DATA;
		else
			HT1632C_PORT &= ~HT1632C_DATA;

		BIT_SLEEP;
		HT1632C_PORT |= HT1632C_WRCLK;
		BIT_SLEEP;

		mask >>= 1;
	}
}

/* gcc does not inline ht1532c_bits_mask, because it's huge. But
 * the 1 << (n-1) is actually pretty complicated on
 * AVR { implemented as while(i) k<<= 1 }.
 */
#define HT1632C_BITS(bits, n) ht1632c_bits_mask((bits), 1 << ((n)-1))

/* send a 8-bit command to the LED controller */
void ht1632c_cmd(uint8_t cmd)
{
	ht1632c_start();
	HT1632C_BITS(0x04, 3); /* 1 0 0 */
	HT1632C_BITS(cmd, 8); /* ... command ... */
	HT1632C_BITS(0, 1); /* ... dummy? ... */
	ht1632c_stop();
}

void ht1632c_bright(uint8_t val)
{
	ht1632c_cmd(0xa0 | (val & 0x0f)); /* 101X-vvvv-X */
}

void ht1632c_onoff(uint8_t val)
{
	ht1632c_cmd(0x00 | !!val); /* 0000-0000-X and 0000-0001-X */
}

void ht1632c_ledonoff(uint8_t val)
{
	ht1632c_cmd(0x02 | !!val); /* 0000-0010-X and 0000-0011-X */
}

void ht1632c_blinkonoff(uint8_t val)
{
	ht1632c_cmd(0x08 | !!val); /* 0000-1000-X and 0000-1001-X */
}

void ht1632c_slave(uint8_t val)
{
	val = val ? 0x04 : 0;
	ht1632c_cmd(0x10 | val); /* 0001-00XX-X and 0001-01XX-X */
}

void ht1632c_clock(uint8_t val)
{
	val = val ? 0x04 : 0;
	ht1632c_cmd(0x18 | val); /* 0001-10XX-X and 0001-11XX-X */
}

void ht1632c_opts(uint8_t val)
{
	val = (val & 0x03) << 2;
	ht1632c_cmd(0x20 | val); /* 0010-abXX-X and 0001-11XX-X */
}

/* write 4 bits of data to LED matrix controller */
void ht1632c_data4(uint8_t addr, uint8_t nibble)
{
	ht1632c_start();
	HT1632C_BITS(0x05, 3); /* 1 0 1 */
	HT1632C_BITS(addr, 7); /* ... command ... */
	HT1632C_BITS(nibble, 4);
	ht1632c_stop();
}

/* write 8 bits to address addr & addr + 1 */
void ht1632c_data8(uint8_t addr, uint8_t byte)
{
	ht1632c_start();
	HT1632C_BITS(0x05, 3); /* 1 0 1 */
	HT1632C_BITS(addr, 7); /* ... command ... */
	HT1632C_BITS(byte, 8);
	ht1632c_stop();
}

void ht1632c_clear_fb(uint8_t *fbmem)
{
	uint8_t i;
	for (i = 0; i < HT1632C_WIDTH; i++)
		*fbmem++ = 0;
}

/* flush a 32x8 framebuffer to the LED matrix */
void ht1632c_flush_fb(uint8_t *fbmem)
{
	uint8_t addr = 0;
	uint8_t fbbit = 0x80;
	uint8_t ledbit;

	ht1632c_start();
	HT1632C_BITS(0x05, 3); /* 1 0 1 */
	HT1632C_BITS(addr, 7); /* ... command ... */

	for (addr = 0; addr < 64; addr += 2) {
		/* for one 8-pixel-block from left to right
		 * the framebuffer (byte) address increases and
		 * the LED matrix controller bit number decreases, so...
		 */

		ledbit = 0x80; /* start MSB */
		while (ledbit) {
			HT1632C_PORT &= ~HT1632C_WRCLK;
			if (*fbmem & fbbit)
				HT1632C_PORT |= HT1632C_DATA;
			else
				HT1632C_PORT &= ~HT1632C_DATA;
			fbmem++; /* next column in FB */
			ledbit >>= 1; /* next column in LED controller */
			HT1632C_PORT |= HT1632C_WRCLK;
		}
		fbmem -= 8; /* move back FB memory pointer */

		fbbit >>= 1; /* move to next row in FB */
		if (!fbbit) { /* reached bottom row?... */
			fbmem += 8; /* move to next block */
			fbbit = 0x80; /* start at 1 */
		}
	}

	ht1632c_stop();
}

void ht1632c_init(void)
{
	uint8_t mask = HT1632C_WRCLK | HT1632C_CS | HT1632C_DATA;
	int i;

	HT1632C_PORT |= mask;
	HT1632C_DDR |= mask;

	ht1632c_start();
	ht1632c_stop();

	ht1632c_onoff(0);
	ht1632c_onoff(1);
	ht1632c_slave(1); /* master mode */
	ht1632c_clock(0); /* internal RC clock */
	ht1632c_opts(0); /* 0: 8 commons, n-mos outputs */
	ht1632c_bright(7);

	/* clear buffer memory */
	for (i = 0; i < 64; i++)
		ht1632c_data4(i, i);

	ht1632c_ledonoff(1); /* turn on */
}
