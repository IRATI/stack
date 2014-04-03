#!/bin/bash

# prepare the guest

set -x

sudo modprobe virtio-ring
sudo modprobe virtio-pci
sudo insmod virtio-mpi.ko
sudo chmod a+rwx /dev/vmpi-test
