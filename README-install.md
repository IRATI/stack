Packages required to install the stack into Debian/Linux 7.0:

Requirements:
  To compile the kernel parts:
     - kernel-package 
     - libncurses5-dev 
     - fakeroot 
     - wget 
     - bzip2
     - bc
     - gcc

  To compile the user-space parts:
     - maven 
     - libnl-genl-3-dev 
     - libnl-3-dev 
     - pkg-config
     - openjdk-6-jdk
     - Latest SWIG version from http://swig.org (versions < 2.0.4 or > 2.0.8)

  To bootstrap the user-space parts:
     - automake
     - autoconf
     - libtool
     - pkg-config
     - git

  To configure the system for the shim-eth-vlan (runtime)
     - vlan
