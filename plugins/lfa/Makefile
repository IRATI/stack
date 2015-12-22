#
# Written by Francesco Salvestrini <f.salvestrini@nextworks.it>
#

ifneq ($(KERNELRELEASE),)

ccflags-y = -Wtype-limits -Inet/rina

obj-m := pff-lfa.o

pff-lfa-y := ps.o

else

ifndef KDIR
$(warning "KDIR will be set to the linux directory of the repository")
KDIR=../../linux
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
	cp pff-lfa.manifest /lib/modules/$(KREL)/extra/

uninstall:
	@echo "This target has not been implemented yet"
	@exit 1

endif
