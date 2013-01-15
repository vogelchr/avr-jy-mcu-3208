DEVICE_CC = atmega8
DEVICE_DUDE = m8

# PROGRAMMER_DUDE = -Pusb -c dragon_isp
PROGRAMMER_DUDE = -P/dev/parport0 -c xil

AVRDUDE=avrdude
OBJCOPY=avr-objcopy
OBJDUMP=avr-objdump
CC=avr-gcc
LD=avr-gcc

LDFLAGS=-Wall -g -mmcu=$(DEVICE_CC)
CPPFLAGS=
CFLAGS=-mmcu=$(DEVICE_CC) -Os -Wall -g -DF_CPU=1000000

MYNAME=jy_mcu

FONTS=font6x8.png

OBJS=$(MYNAME).o ht1632c.o serial.o $(FONTS:.png=.o) fonts.o

all : $(MYNAME).hex $(MYNAME).lst

$(MYNAME).bin : $(OBJS)

%.hex : %.bin
	$(OBJCOPY) -j .text -j .data -O ihex $^ $@ || (rm -f $@ ; false )

%.lst : %.bin
	$(OBJDUMP) -S $^ >$@ || (rm -f $@ ; false )

%.bin : %.o
	$(LD) $(LDFLAGS) -o $@ $^

%.c : %.txt %.png
	./png_to_font.py $*.txt $*.png >$@ || (rm -f $@ ; false)

ifneq "$(MAKECMDGOALS)" "clean" 
include $(OBJS:.o=.d)
endif

%.d : %.c
	$(CC) -o $@ -MM $^

.PHONY : clean burn
burn : $(MYNAME).hex
	$(AVRDUDE) $(PROGRAMMER_DUDE) -p $(DEVICE_DUDE) -U flash:w:$^
clean :
	rm -f *.bak *~ *.bin *.hex *.lst *.o *.d $(FONTS:.png=.c)
