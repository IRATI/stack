directories=$(shell ls)

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
		cd $$i || continue; 						\
		make KDIR=$(KDIR) INSTALLDIR=$(INSTALLDIR) KREL=$(KREL); 	\
		cd ..; 								\
	done

clean:
	for i in $(directories); do 						\
		cd $$i || continue; 						\
		make KDIR=$(KDIR) INSTALLDIR=$(INSTALLDIR) KREL=$(KREL) clean; 	\
		cd ..; 								\
	done

install:
	for i in $(directories); do 						\
		cd $$i || continue; 						\
		make KDIR=$(KDIR) INSTALLDIR=$(INSTALLDIR) KREL=$(KREL) install;\
		cd ..; 								\
	done
