#!/bin/bash

set -x

# unprepare the host

sudo rmmod vmpi-host-virtio
sudo rmmod shim-hv
