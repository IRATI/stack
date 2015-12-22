#
# Makefile for IRATI Explicit Congestion Notification policy.
#
# Author: Kewin Rausch <kewin.rausch@create-net.org>
#

KDIR=../../../linux
KREL=$(shell cd $(KDIR) && make kernelrelease)

EXTRA_CFLAGS=-Wtype-limits -Inet/rina/

obj-m	+= cdrr.o
cdrr-y	:= dtcp-ps-cdrr.o

all:
	$(MAKE) -C $(KDIR) M=$(shell pwd)
	
install:
	$(MAKE) -C $(KDIR) M=$(shell pwd) modules_install
	cp cdrr.manifest /lib/modules/$(KREL)/extra/
	
clean:
	$(MAKE) -C $(KDIR) M=$(shell pwd) clean
