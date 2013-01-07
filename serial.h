#ifndef SERIAL_H
#define SERIAL_H

/* 1 if there's data to be read from stdin */
extern int serial_status(void);

/* initialize uart, setup interrupts, connect stdin/stdout */
extern void serial_init(void);

#endif
