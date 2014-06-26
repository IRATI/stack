#!/bin/bash

set -x

sudo modprobe shim-hv
sudo modprobe vmpi-kvm-host
sudo chmod a+rwx /dev/vhost-mpi
