#
# Written by Francesco Salvestrini <f.salvestrini@nextworks.it>
#

ifneq ($(KERNELRELEASE),)

ccflags-y = -Wtype-limits -Inet/rina

obj-m := dummy-plugin.o

dummy-plugin-y := dummy-plugin-ps.o rmt-ps-dummy.o dtcp-ps-dummy.o dtp-ps-dummy.o pft-ps-dummy.o

else

ifndef KDIR
$(warning "KDIR will be set to the linux directory of the repository")
KDIR=../../linux
endif

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
