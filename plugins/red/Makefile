#
# Written by Francesco Salvestrini <f.salvestrini@nextworks.it>
#

ifneq ($(KERNELRELEASE),)

ccflags-y = -Wtype-limits -Inet/rina

obj-m := red-plugin.o

red-plugin-y := rmt-ps-debug.o dtcp-ps-debug.o red-ps.o rmt-ps-red.o dtcp-ps-red.o

else

ifndef KDIR
$(warning "KDIR will be set to the linux directory of the repository")
KDIR=../../linux
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
	cp red-plugin.manifest /lib/modules/$(KREL)/extra/

uninstall:
	@echo "This target has not been implemented yet"
	@exit 1

endif
