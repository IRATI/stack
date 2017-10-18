directories=$(shell ls -d */)

ifndef INSTALLDIR
$(error "INSTALLDIR is not set")
endif

ifndef KDIR
$(error "KDIR is not set")
endif

ifndef KREL
$(error "KREL is not set")
endif

all:
	for i in $(directories); do 						\
		cd $$i || exit 1; 						\
		make KDIR=$(KDIR) INSTALLDIR=$(INSTALLDIR) KREL=$(KREL) || exit 1; 	\
		cd ..; 								\
	done

clean:
	for i in $(directories); do 						\
		cd $$i || exit 1; 						\
		make KDIR=$(KDIR) INSTALLDIR=$(INSTALLDIR) KREL=$(KREL) clean || exit 1; 	\
		cd ..; 								\
	done

install:
	for i in $(directories); do 						\
		cd $$i || exit 1; 						\
		make KDIR=$(KDIR) INSTALLDIR=$(INSTALLDIR) KREL=$(KREL) install || exit 1;\
		cd ..; 								\
	done
