#!/bin/bash

set -x

# unprepare the host

sudo rmmod vmpi-virtio-host
sudo rmmod shim-hv
