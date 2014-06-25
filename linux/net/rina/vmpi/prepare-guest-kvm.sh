#!/bin/bash

set -x

sudo modprobe shim-hv
sudo modprobe vmpi-kvm-guest
