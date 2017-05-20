#!/bin/bash

# unprepare the guest

set -x

sudo rmmod vmpi-guest-xen
sudo rmmod vmpi-provider
sudo rmmod vmpi-bufs
