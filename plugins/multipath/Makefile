#
# Written by Francesco Salvestrini <f.salvestrini@nextworks.it>
#

KDIR=../../linux

ifneq ($(KERNELRELEASE),)

ccflags-y = -Wtype-limits -Inet/rina

obj-m := mp-plugin.o

mp-plugin-y := mp-plugin-ps.o pft-ps-multipath.o

else

all:
	$(MAKE) -C $(KDIR) M=$$PWD

clean:
	rm -r -f *.o *.ko *.mod.c *.mod.o Module.symvers .*.cmd .tmp_versions modules.order

install:
	$(MAKE) -C $(KDIR) M=$$PWD modules_install
	cp mp-plugin.manifest /lib/modules/3.15.0-pristine-1.3-mp/extra/

uninstall:
	@echo "This target has not been implemented yet"
	@exit 1

endif
