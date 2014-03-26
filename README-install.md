Packages required to install the stack into Debian/Linux 7.0:

Requirements:
  * To compile the kernel parts:
     * kernel-package 
       * libncurses5-dev [for 'make menuconfig']
     * fakeroot 
     * wget 
     * bzip2
     * bc

  * To compile the user-space parts:
     * autoconf
     * automake
     * libtool
     * pkg-config
     * git
     * libnl-genl-3-dev 
     * libnl-3-dev 
     * maven 
     * openjdk-6-jdk
     * SWIG version 2.x, from http://swig.org (>2.0.8 required)
        * libpcre3-dev [or configure SWIG using the --without-pcre option]

  * To use shim-eth-vlan
     * vlan
