# Aggregate (longest-prefix) PDU forwarding policy set.

ifndef KREL
KREL=`uname -r`
endif

ifndef KDIR
KDIR=/lib/modules/$(KREL)/build
endif

ifndef IRATI_KSDIR
IRATI_KSDIR=${PWD}/../../kernel
endif

ccflags-y = -Wtype-limits -I${src}/../../kernel -I${src}/../../include

obj-m := pff-aggregate.o

all:
	$(MAKE) -C $(KDIR) KBUILD_EXTRA_SYMBOLS=${IRATI_KSDIR}/Module.symvers M=$$PWD

clean:
	rm -r -f *.o *.ko *.mod.c *.mod.o Module.symvers .*.cmd .tmp_versions modules.order

install:
	$(MAKE) -C $(KDIR) M=$$PWD modules_install
	cp pff-aggregate.manifest /lib/modules/$(KREL)/extra/
	depmod -a

uninstall:
	@echo "This target has not been implemented yet"
	@exit 1
