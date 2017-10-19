directories=$(shell ls -d */)

ifndef KREL
KREL=`uname -r`
endif

ifndef KDIR
KDIR=/lib/modules/$(KREL)/build
endif

ifndef IRATI_KSDIR
IRATI_KSDIR=${PWD}/../kernel
endif

all:
	for i in $(directories); do 						\
		cd $$i || exit 1; 						\
		make INSTALLDIR=$(INSTALLDIR) KDIR=$(KDIR) IRATI_KSDIR=$(IRATI_KSDIR) KREL=$(KREL) || exit 1; 	\
		cd ..; 								\
	done

clean:
	for i in $(directories); do 						  \
		cd $$i || exit 1; 						  \
		make INSTALLDIR=$(INSTALLDIR) KDIR=$(KDIR) IRATI_KSDIR=$(IRATI_KSDIR) KREL=$(KREL) clean || exit 1;  \
		cd ..; 								  \
	done

install:
	for i in $(directories); do 						  \
		cd $$i || exit 1; 						  \
		make INSTALLDIR=$(INSTALLDIR) KDIR=$(KDIR) IRATI_KSDIR=$(IRATI_KSDIR) KREL=$(KREL) install || exit 1;\
		cd ..; 								  \
	done
