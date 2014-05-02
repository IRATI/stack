#!/bin/bash

set -x

# prepare the host

sudo modprobe vhost
sudo insmod ./shim-hv.ko
sudo insmod ../vmpi/vmpi-virtio-host.ko
sudo chmod a+rwx /dev/vhost-mpi
