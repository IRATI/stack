#
# Written by Francesco Salvestrini <f.salvestrini@nextworks.it>
#

KDIR=../../linux
KREL=`uname -r`

ifneq ($(KERNELRELEASE),)

ccflags-y = -Wtype-limits -Inet/rina

obj-m := pff-multipath.o

pff-multipath-y := mp-plugin-ps.o pft-ps-multipath.o

else

all:
	$(MAKE) -C $(KDIR) M=$$PWD

clean:
	rm -r -f *.o *.ko *.mod.c *.mod.o Module.symvers .*.cmd .tmp_versions modules.order

install:
	$(MAKE) -C $(KDIR) M=$$PWD modules_install
	cp pff-multipath.manifest /lib/modules/$(KREL)/extra/

uninstall:
	@echo "This target has not been implemented yet"
	@exit 1

endif
