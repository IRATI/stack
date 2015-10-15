#!/bin/bash

# prepare the guest

set -x

sudo modprobe virtio-ring
sudo modprobe virtio-pci

lsmod | grep vmpi_bufs || sudo insmod vmpi-bufs.ko
lsmod | grep vmpi_provider || sudo insmod vmpi-provider.ko
lsmod | grep vmpi_guest_kvm || sudo insmod vmpi-guest-kvm.ko

lsmod | grep vmpi_test || sudo insmod vmpi-test.ko
[ -c /dev/vmpi-test ] && sudo chmod a+rwx /dev/vmpi-test
