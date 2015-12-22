#
# Written by Francesco Salvestrini <f.salvestrini@nextworks.it>
#

KDIR=../../linux
KREL=$(shell cd $(KDIR) && make kernelrelease)

ifneq ($(KERNELRELEASE),)

ccflags-y = -Wtype-limits -Inet/rina

obj-m := cherish-urgency-plugin.o

cherish-urgency-plugin-y := cherish-urgency.o

else

all:
	$(MAKE) -C $(KDIR) M=$$PWD

clean:
	rm -r -f *.o *.ko *.mod.c *.mod.o Module.symvers .*.cmd .tmp_versions modules.order

install:
	$(MAKE) -C $(KDIR) M=$$PWD modules_install
	cp cherish-urgency-plugin.manifest /lib/modules/$(KREL)/extra/

uninstall:
	@echo "This target has not been implemented yet"
	@exit 1

endif
