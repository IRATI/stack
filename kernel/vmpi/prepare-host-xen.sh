#!/bin/bash


usage()
{
        echo "USAGE:    $0 <FRONTEND-DOM-ID>"
        exit
}


echo "$1" | grep -o "^[0-9]\+$"
if test "$?" != 0; then
        usage
fi

FRONTEND_ID="$1"  # DomID of the frontend we want to setup

# Load the vmpi-host-xen module, if not already loaded
lsmod | grep vmpi_provider || sudo insmod vmpi-bufs.ko
lsmod | grep vmpi_provider || sudo insmod vmpi-provider.ko
lsmod | grep vmpi_host_xen || sudo insmod vmpi-host-xen.ko

# Write backend information into the location the frontend will look
# for it.
sudo xenstore-write /local/domain/${FRONTEND_ID}/device/mpi/0/backend-id 0
sudo xenstore-write /local/domain/${FRONTEND_ID}/device/mpi/0/backend \
                /local/domain/0/backend/mpi/${FRONTEND_ID}/0

# Write frontend information into the location the backend will look
# for it.
sudo xenstore-write /local/domain/0/backend/mpi/${FRONTEND_ID}/0/frontend-id ${FRONTEND_ID}
sudo xenstore-write /local/domain/0/backend/mpi/${FRONTEND_ID}/0/frontend \
                /local/domain/${FRONTEND_ID}/device/mpi/0

# Set the permissions on the backend so that the frontend can
# actually read it.
sudo xenstore-chmod /local/domain/0/backend/mpi/${FRONTEND_ID}/0 r
sudo xenstore-chmod -r /local/domain/${FRONTEND_ID}/device/mpi/0 b   # sed 's|b|w'

# Write the states.  Note that the backend state must be written
# last because it requires a valid frontend state to already be
# written.
sudo xenstore-write /local/domain/${FRONTEND_ID}/device/mpi/0/state 1
sudo xenstore-write /local/domain/0/backend/mpi/${FRONTEND_ID}/0/state 1
