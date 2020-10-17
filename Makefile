DEVICE_CC = atmega8
DEVICE_DUDE = m8

PROGRAMMER_DUDE = -Pusb -c dragon_isp -B1.0 -v
# PROGRAMMER_DUDE = -P/dev/parport0 -c xil

AVRDUDE=avrdude
OBJCOPY=avr-objcopy
OBJDUMP=avr-objdump
SIZE=avr-size
CC=avr-gcc
LD=avr-gcc

# LTO=-flto

LDFLAGS=-Wall -g $(LTO) -mmcu=$(DEVICE_CC)
CPPFLAGS=
CFLAGS=-mmcu=$(DEVICE_CC) -Os -Wall -Wextra -g3 -ggdb $(LTO) -DF_CPU=8000000

MYNAME=jy_mcu

FONTS=font6x8.png font4x5.png

OBJS=$(MYNAME).o ht1632c.o serial.o $(FONTS:.png=.o) fonts.o clock_ui.o clock_timer.o menu_items.o sysconfig.o

all : $(MYNAME).hex $(MYNAME).lst

$(MYNAME).bin : $(OBJS)

%.hex : %.bin
	$(OBJCOPY) -j .text -j .data -O ihex $^ $@ || (rm -f $@ ; false )

%.lst : %.bin
	$(OBJDUMP) -rS $^ >$@ || (rm -f $@ ; false )
	$(SIZE) $^

%.bin : %.o
	$(LD) $(LDFLAGS) -o $@ $^

%.c : %.txt %.png
	./png_to_font.py $*.txt $*.png >$@ || (rm -f $@ ; false)

menu_items.c : generate_menu.py
	./generate_menu.py $@

ifneq "$(MAKECMDGOALS)" "clean" 
include $(OBJS:.o=.d)
endif

%.d : %.c
	$(CC) -o $@ -MM $^

.PHONY : clean burn fuse
burn : $(MYNAME).hex
	$(AVRDUDE) $(PROGRAMMER_DUDE) -p $(DEVICE_DUDE) -U flash:w:$^
fuse :
	$(AVRDUDE) $(PROGRAMMER_DUDE) -p $(DEVICE_DUDE) -U lfuse:w:0xe4:m -U hfuse:w:0xC9:m

clean :
	rm -f *.bak *~ *.bin *.hex *.lst *.o *.d $(FONTS:.png=.c) menu_items.c
