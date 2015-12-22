#
# Makefile for IRATI Congestion Detection Rate Reduction policy.
#
# Author: Kewin Rausch <kewin.rausch@create-net.org>
#

KDIR=../../../linux
KREL=$(shell cd $(KDIR) && make kernelrelease)

EXTRA_CFLAGS=-Wtype-limits -Inet/rina/

obj-m	+= ecn.o
ecn-y	:= rmt-ps-ecn.o

all:
	$(MAKE) -C $(KDIR) M=$(shell pwd)

install:
	$(MAKE) -C $(KDIR) M=$(shell pwd) modules_install
	cp ecn.manifest /lib/modules/$(KREL)/extra/

clean:
	$(MAKE) -C $(KDIR) M=$(shell pwd) clean
