#!/bin/bash


set-x
# To be executed in the vmpi folder
IRATI_REPO=~/git/irati

rm *.patch

git format-patch rina..master
patches=$(ls *.patch)
mv ${patches} ${IRATI_REPO}

pushd ${IRATI_REPO}
for patch in ${patches}; do
        git am --directory=linux/net/rina/vmpi --exclude=linux/net/rina/vmpi/user-vmpi-test.c --exclude=linux/net/rina/vmpi/*.sh ${patch}
done
rm *.patch
popd

#git checkout rina
#git merge master

