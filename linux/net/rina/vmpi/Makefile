#obj-m += vhost-mpi.o
#vhost-mpi-y := vmpi-host-impl-virtio.o vmpi-host.o vmpi-ring.o vmpi-host-test.o

#obj-m += virtio-mpi.o
#virtio-mpi-y := vmpi-guest-impl-virtio.o vmpi-guest-test.o vmpi-guest.o vmpi-ring.o

KER=`uname -r`

all: user-guest-test
	make -C /usr/lib/modules/$(KER)/build M=$(PWD) modules

user-guest-test: user-guest-test.o
	gcc -Wall -g -o user-guest-test user-guest-test.o -lrt

user-guest-test.o: user-guest-test.c
	gcc -Wall -g -c user-guest-test.c

clean:
	make -C /usr/lib/modules/$(KER)/build M=$(PWD) clean
