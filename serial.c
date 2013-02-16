#include "serial.h"

#include <stdio.h>
#include <avr/interrupt.h>

#define USE_U2X 1
#define UBRR_CALC(baud,f_osc,u2x)	(((uint32_t)(f_osc)/(8*(2-(u2x))*(uint32_t)(baud)))-1)

/* static FILE structure for stdout */
static int uart_putchar(char c, FILE *stream);
static int uart_getchar(FILE *stream);

static FILE uart_stdout = FDEV_SETUP_STREAM(uart_putchar, NULL, _FDEV_SETUP_WRITE);
static FILE uart_stdin  = FDEV_SETUP_STREAM(NULL, uart_getchar, _FDEV_SETUP_READ);

#define UART_BUFSIZE    32

static char uart_outbuf[UART_BUFSIZE];
static volatile uint8_t uart_writep;
static volatile uint8_t uart_readp;

ISR(USART_UDRE_vect){
	uint8_t rp = uart_readp;
	UDR = uart_outbuf[rp];

	rp = (rp + 1)%UART_BUFSIZE;
	uart_readp = rp;

	if(rp == uart_writep){ /* empty */
		UCSRB &= ~ _BV(UDRIE); /* turn off Data Register Empty Interrupt */
	}
}

static inline void
uart_put_into_outbuf(unsigned char c){
	uint8_t wp;

	while(1){
		cli();
		wp = uart_writep;
		uart_outbuf[wp]=c;
		wp = (wp + 1)%UART_BUFSIZE;
		if(wp == uart_readp){ /* full! */
			sei();
			continue;
		}
		uart_writep = wp;
		if(!(UCSRB & _BV(UDRIE))) /* if Data Register Empty Interrupt... */
			UCSRB |= _BV(UDRIE); /* is off, turn it on. */
		sei();
		break;
	}
}

static int
uart_putchar(char c, FILE *stream)
{
	(void) stream; /* silence warning */

	if(c == '\r')
		return 0;
	if(c == '\n')
		uart_put_into_outbuf('\r');
	uart_put_into_outbuf(c);
	return 0;
}

static int
uart_getchar(FILE *stream)
{
	(void) stream; /* silence warning */

	/* wait for Receive Complete bit to be set */
	while(!serial_status()); /* wait... */
	return UDR;
}

/* there is a byte to be read */
int
serial_status(){
	return UCSRA & _BV(RXC);
}

static void
write_ubrr(uint16_t v){
	/* UCSRC is UBRRH for writing if MSB is not set! */
	UCSRC = (v >> 8) & ~ 0x80;
	UBRRL =  v & 0xff;
}

static void
write_ucsrc(uint8_t v){
	v |= 0x80;
	UCSRC = v;
}

void
serial_init(){

	UCSRA = (USE_U2X ? _BV(U2X) : 0);
	UCSRB = _BV(RXEN)|_BV(TXEN);

	/* UCSRC is a bit special... */
	write_ucsrc(_BV(UCSZ1)|_BV(UCSZ0)); /* 8 bit, no parity, 1 stopbit */
	write_ubrr(UBRR_CALC(9600,F_CPU,USE_U2X));

	stdout = &uart_stdout;
	stdin  = &uart_stdin;
}
