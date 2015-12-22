directories=$(shell ls)

ifndef INSTALLDIR
$(error "INSTALLDIR is not set")
endif

ifndef KDIR
$(warning "KDIR will be set to the linux directory of the repository")
KDIR=$(shell echo `pwd`/../linux)
endif

KREL=$(shell make -s -C $(KDIR) kernelrelease)

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
