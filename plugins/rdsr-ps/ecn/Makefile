#
# Makefile for IRATI Congestion Detection Rate Reduction policy.
#
# Author: Kewin Rausch <kewin.rausch@create-net.org>
#

ifneq ($(KERNELRELEASE),)

ccflags-y = -Wtype-limits -Inet/rina

obj-m	+= ecn.o
ecn-y	:= rmt-ps-ecn.o

else

ifndef KDIR
$(warning "KDIR will be set to the linux directory of the repository")
KDIR=../../../linux
endif

ifndef
KREL=$(shell make -s -C $(KDIR) kernelrelease)
endif

all:
	$(MAKE) -C $(KDIR) M=$$PWD

clean:
	rm -r -f *.o *.ko *.mod.c *.mod.o Module.symvers .*.cmd .tmp_versions modules.order

install:
	$(MAKE) -C $(KDIR) M=$$PWD modules_install
	cp ecn.manifest /lib/modules/$(KREL)/extra/

uninstall:
	@echo "This target has not been implemented yet"
	@exit 1

endif
