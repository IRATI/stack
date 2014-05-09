#!/bin/bash

set -x

# prepare the host

sudo modprobe vhost
sudo insmod vhost-mpi.ko
sudo chmod a+rwx /dev/vhost-mpi
