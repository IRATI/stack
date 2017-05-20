#!/bin/bash

# prepare the guest

set -x

sudo insmod vmpi-bufs.ko
sudo insmod vmpi-provider.ko
sudo insmod vmpi-guest-xen.ko
