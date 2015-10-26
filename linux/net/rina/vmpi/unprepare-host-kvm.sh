#!/bin/bash

set -x

# unprepare the host

lsmod | grep vmpi_test && sudo rmmod vmpi-test
lsmod | grep vmpi_host_kvm && sudo rmmod vmpi-host-kvm
lsmod | grep vmpi_provider && sudo rmmod vmpi-provider
lsmod | grep vmpi_bufs && sudo rmmod vmpi-bufs
