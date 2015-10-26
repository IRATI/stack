#!/bin/bash

# unprepare the guest

set -x

lsmod | grep vmpi_test && sudo rmmod vmpi-test
lsmod | grep vmpi_guest_kvm && sudo rmmod vmpi-guest-kvm
lsmod | grep vmpi_provider && sudo rmmod vmpi-provider
lsmod | grep vmpi_bufs && sudo rmmod vmpi-bufs
