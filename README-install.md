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

  * For the kernel parts:
     * kernel-package
       * libncurses5-dev [for 'make menuconfig']

  * For the user-space parts:
     * autoconf
     * automake
     * libtool
     * pkg-config
     * git

     * g++
     * libnl-genl-3-dev 
     * libnl-3-dev 

     * openjdk-6-jdk
     * maven 

     * SWIG version 2.x, from http://swig.org
        * version >2.0.8 required, version 2.0.12 is known to be working fine
        * Depending on your setup, libpcre3-dev might be required. If you
          don't want its support, just configure swig without it by passing
          the --without-pcre option to its `configure' script.

  * To use shim-eth-vlan
     * vlan

Have fun,
Francesco
