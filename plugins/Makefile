directories=$(shell ls)

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
		cd $$i || continue; 						\
		make KDIR=$(KDIR) IRATI_KSDIR=$(IRATI_KSDIR) KREL=$(KREL); 	\
		cd ..; 								\
	done

clean:
	for i in $(directories); do 						  \
		cd $$i || continue; 						  \
		make KDIR=$(KDIR) IRATI_KSDIR=$(IRATI_KSDIR) KREL=$(KREL) clean;  \
		cd ..; 								  \
	done

install:
	for i in $(directories); do 						  \
		cd $$i || continue; 						  \
		make KDIR=$(KDIR) IRATI_KSDIR=$(IRATI_KSDIR) KREL=$(KREL) install;\
		cd ..; 								  \
	done
