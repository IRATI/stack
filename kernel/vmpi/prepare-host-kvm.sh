#!/bin/bash

set -x

# prepare the host

sudo modprobe vhost

lsmod | grep vmpi_bufs || sudo insmod vmpi-bufs.ko
lsmod | grep vmpi_provider || sudo insmod vmpi-provider.ko
lsmod | grep vmpi_host_kvm || sudo insmod vmpi-host-kvm.ko
[ -c /dev/vhost-mpi ] && sudo chmod a+rwx /dev/vhost-mpi

lsmod | grep vmpi_test || sudo insmod vmpi-test.ko
[ -c /dev/vmpi-test ] && sudo chmod a+rwx /dev/vmpi-test
