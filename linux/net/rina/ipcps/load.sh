#!/bin/bash

set -x

# prepare the host

sudo modprobe vhost
sudo insmod ./shim-hv-host-virtio.ko
sudo chmod a+rwx /dev/vhost-mpi
