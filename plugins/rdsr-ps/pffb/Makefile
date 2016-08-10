#
# Makefile for IRATI PDU Forwarding Flow Balancing policy.
#
# Author: Kewin Rausch <kewin.rausch@create-net.org>
#

KDIR=/pristine/linux

ifneq ($(KERNELRELEASE),)

ccflags-y = -Wtype-limits -Inet/rina

obj-m	+= pffb.o
pffb-y	:= pff-ps-pffb.o

else

ifndef KDIR
$(warning "KDIR will be set to the linux directory of the repository")
KDIR=../../../linux
endif

ifndef KREL
KREL=$(shell make -s -C $(KDIR) kernelrelease)
endif

all:
	$(MAKE) -C $(KDIR) M=$$PWD

clean:
	rm -r -f *.o *.ko *.mod.c *.mod.o Module.symvers .*.cmd .tmp_versions modules.order

install:
	$(MAKE) -C $(KDIR) M=$$PWD modules_install
	cp pffb.manifest /lib/modules/$(KREL)/extra/
	depmod -a

uninstall:
	@echo "This target has not been implemented yet"
	@exit 1

endif
