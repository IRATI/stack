#!/bin/bash


usage()
{
        echo "USAGE:    $0 <FRONTEND-DOM-ID>"
        exit
}

cleandir()
{
        local param=${1}
        local key
        local result

        result=$(sudo xenstore-list ${param})
        for key in ${result}; do
               sudo xenstore-rm ${param}/${key}
        done
}


echo "$1" | grep -o "^[0-9]\+$"
if test "$?" != 0; then
        usage
fi

sudo rmmod vmpi-host-xen
sudo rmmod vmpi-provider
sudo rmmod vmpi-bufs

FRONTEND_ID="$1"  # DomID of the frontend we want to teardown

cleandir /local/domain/${FRONTEND_ID}/device/mpi/0
cleandir /local/domain/0/backend/mpi/${FRONTEND_ID}/0

