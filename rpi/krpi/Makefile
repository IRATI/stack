#
# Written by Francesco Salvestrini <f.salvestrini@nextworks.it>
#

KDIR=../../linux

ifneq ($(KERNELRELEASE),)

ccflags-y = -Wtype-limits

obj-m := skeleton.o

skeleton-y := source1.o source2.o

else

all:
	$(MAKE) -C $(KDIR) M=$$PWD

clean:
	rm -r -f *.o *.ko *.mod.c *.mod.o Module.symvers .*.cmd .tmp_versions modules.order

endif
