This repository holds a RINA stack implementation

This work is partially funded by the EU FP7 IRATI Project

Packages needed in distro debian 7.0:
        
To compile kernel:
   - kernel-package 
   - libncurses5-dev 
   - fakeroot 
   - wget 
   - bzip2

To compile user-space:
   - maven 
   - libnl-genl-3-dev 
   - libnl-3-dev 
   - libtool 
   - autoconf2.13
   - openjdk-6-jdk
   - git
   - Latest SWIG version from http://swig.org

Compile and install kernel: 

   - sudo make menuconfig --> Enable RINA features
   - sudo make bzImage modules -j[number of cores] && sudo make modules_install install

Compile and install user-space: 

   - ./install-from-sratch [absolute path where you want the binaries to be installed]
