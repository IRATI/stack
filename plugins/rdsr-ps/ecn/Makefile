#
# Makefile for IRATI Congestion Detection Rate Reduction policy.
#
# Author: Kewin Rausch <kewin.rausch@create-net.org>
#

EXTRA_CFLAGS=-Wtype-limits -Inet/rina/

obj-m	+= ecn.o
ecn-y	:= rmt-ps-ecn.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd)

install:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules_install
	cp ecn.manifest /lib/modules/$(shell uname -r)/extra/

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) clean