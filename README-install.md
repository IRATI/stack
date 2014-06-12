Packages required to install the stack into Debian/Linux 7.0 (wheezy):

1) Update libnl/libnl-genl (>= 3.2.14 required):

  1.a) Add 'deb http://ftp.de.debian.org/debian jessie main' in
       /etc/apt/sources.list
  1.b) Run 'apt-get update'
  1.c) Run 'apt-get install libnl-genl-3-dev'

2) Fullfil the following requirements by running
   'apt-get install <package>' OR compiling the packages from sources. In the
   latter, system-wide installation is assumed (please follow per-package
   installation instructions):

  * To compile the kernel parts:
     * kernel-package 
       * libncurses5-dev [for 'make menuconfig']

  * To compile the user-space parts:
     * autoconf
     * automake
     * libtool
     * pkg-config
     * git
     * g++
     * libnl-genl-3-dev 
     * libnl-3-dev 
     * maven 
     * openjdk-6-jdk

     * SWIG version 2.x, from http://swig.org (>2.0.8 required)
        * libpcre3-dev [or configure SWIG using the --without-pcre option]

  * To use shim-eth-vlan
     * vlan [for VLANs setup]

Have fun,
Francesco
