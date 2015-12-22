directories=$(shell ls)

all:
	for i in $(directories); do 			\
		cd $$i || continue; 			\
		INSTALLDIR=$(INSTALLDIR) make; 		\
		cd ..; 					\
	done

clean:
	for i in $(directories); do 			\
		cd $$i || continue; 			\
		INSTALLDIR=$(INSTALLDIR) make clean; 	\
		cd ..; 					\
	done

install:
	for i in $(directories); do 			\
		cd $$i || continue; 			\
		INSTALLDIR=$(INSTALLDIR) make install; 	\
		cd ..; 					\
	done
