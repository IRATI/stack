#
# Written by Francesco Salvestrini <f.salvestrini@nextworks.it>
#
# Author: Kewin Rausch <kewin.rausch@create-net.org>
#

ifndef KREL
KREL=`uname -r`
endif

ifndef KDIR
KDIR=/lib/modules/$(KREL)/build
endif

ifndef IRATI_KSDIR
IRATI_KSDIR=${PWD}/../../../kernel
endif

ccflags-y = -Wtype-limits -I${src}/../../../kernel -I${src}/../../../include

obj-m := dtcp-ps-cdrr.o
qta-mux-plugin-y := cdrr.o

all:
	$(MAKE) -C $(KDIR) KBUILD_EXTRA_SYMBOLS=${IRATI_KSDIR}/Module.symvers M=$$PWD

clean:
	rm -r -f *.o *.ko *.mod.c *.mod.o Module.symvers .*.cmd .tmp_versions modules.order

install:
	$(MAKE) -C $(KDIR) M=$$PWD modules_install
	cp cdrr.manifest /lib/modules/$(KREL)/extra/
	depmod -a

uninstall:
	@echo "This target has not been implemented yet"
	@exit 1
