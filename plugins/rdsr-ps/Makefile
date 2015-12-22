directories=$(shell ls)

ifndef INSTALLDIR
$(error "INSTALLDIR is not set")
endif

ifndef KDIR
$(error "KDIR is not set")
endif

all:
	for i in $(directories); do 					\
		cd $$i || continue; 					\
		KDIR=$(KDIR) INSTALLDIR=$(INSTALLDIR) make; 		\
		cd ..; 							\
	done

clean:
	for i in $(directories); do 					\
		cd $$i || continue; 					\
		KDIR=$(KDIR) INSTALLDIR=$(INSTALLDIR) make clean; 	\
		cd ..; 							\
	done

install:
	for i in $(directories); do 					\
		cd $$i || continue; 					\
		KDIR=$(KDIR) INSTALLDIR=$(INSTALLDIR) make install; 	\
		cd ..; 							\
	done
