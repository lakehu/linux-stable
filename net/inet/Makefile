#
# Makefile for the Linix TCP/IP (INET) layer.
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#
# Note 2! The CFLAGS definition is now in the main makefile...

CFLAGS := $(CFLAGS) -I../socket -I..
CPP := $(CPP) -I../socket -I..

.c.o:
	$(CC) $(CFLAGS) -c -o $*.o $<
.s.o:
	$(AS) -o $*.o $<
.c.s:
	$(CC) $(CFLAGS) -S -o $*.s $<


OBJS	= sockinet.o utils.o route.o proc.o timer.o protocol.o loopback.o \
	  eth.o packet.o arp.o devinet.o ip.o raw.o icmp.o tcp.o udp.o

ifdef CONFIG_INET

inet.o:		$(OBJS)
		$(LD) -r -o inet.o $(OBJS)

else

inet.o:
		echo | $(AS) -o inet.o

endif

dep:
		$(CPP) -M *.c > .depend

tar:
		tar -cvf /dev/f1 .

#
# include a dependency file if one exists
#
ifeq (.depend,$(wildcard .depend))
include .depend
endif
