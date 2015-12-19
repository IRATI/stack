#
# Makefile for IRATI Explicit Congestion Notification policy.
#
# Author: Kewin Rausch <kewin.rausch@create-net.org>
#


EXTRA_CFLAGS=-Wtype-limits -Inet/rina/

obj-m	+= cdrr.o
cdrr-y	:= dtcp-ps-cdrr.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd)
	
install:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules_install
	cp cdrr.manifest /lib/modules/$(shell uname -r)/extra/
	
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) clean