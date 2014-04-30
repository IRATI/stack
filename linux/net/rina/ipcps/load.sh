#!/bin/bash

set -x

# prepare the host

sudo modprobe vhost
sudo modprobe vmpi-host-virtio
sudo chmod a+rwx /dev/vhost-mpi
