#
# Written by Francesco Salvestrini <f.salvestrini@nextworks.it>
#

KDIR=../../linux

ifneq ($(KERNELRELEASE),)

ccflags-y = -Wtype-limits -Inet/rina

obj-m := red-plugin.o

red-plugin-y := rmt-ps-debug.o dtcp-ps-debug.o red-ps.o rmt-ps-red.o dtcp-ps-red.o

else

all:
	$(MAKE) -C $(KDIR) M=$$PWD

clean:
	rm -r -f *.o *.ko *.mod.c *.mod.o Module.symvers .*.cmd .tmp_versions modules.order

install:
	$(MAKE) -C $(KDIR) M=$$PWD modules_install

uninstall:
	@echo "This target has not been implemented yet"
	@exit 1

endif
