#
# Written by Francesco Salvestrini <f.salvestrini@nextworks.it>
#

KDIR=../../linux

ifneq ($(KERNELRELEASE),)

ccflags-y = -Wtype-limits -Inet/rina

obj-m := cas-plugin.o

cas-plugin-y := cas-plugin-ps.o rmt-ps-cas.o dtcp-ps-cas.o dtp-ps-cas.o 

else

all:
	$(MAKE) -C $(KDIR) M=$$PWD

clean:
	rm -r -f *.o *.ko *.mod.c *.mod.o Module.symvers .*.cmd .tmp_versions modules.order

install:
	@echo "This target has not been implemented yet"
	@exit 1

uninstall:
	@echo "This target has not been implemented yet"
	@exit 1

endif
