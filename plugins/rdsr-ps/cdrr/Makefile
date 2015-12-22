#
# Makefile for IRATI Explicit Congestion Notification policy.
#
# Author: Kewin Rausch <kewin.rausch@create-net.org>
#

ifneq ($(KERNELRELEASE),)

ccflags-y = -Wtype-limits -Inet/rina

obj-m	+= cdrr.o
cdrr-y	:= dtcp-ps-cdrr.o

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
	cp cdrr.manifest /lib/modules/$(KREL)/extra/

uninstall:
	@echo "This target has not been implemented yet"
	@exit 1

endif
