Packages required to install the stack into Debian/Linux 7.0 (wheezy):

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
    * libnl-genl-3-dev and libnl-3-dev:
      * Add 'deb http://ftp.de.debian.org/debian jessie main' in
         /etc/apt/sources.list
      * Run 'apt-get update'
      * Run 'apt-get install libnl-genl-3-dev libnl-3-dev'
 
 
    * openjdk-6-jdk
    * maven 
 
    * SWIG version 2.x, from http://swig.org
       * version >2.0.8 required, version 2.0.12 is known to be working fine
       * Depending on your intended setup, libpcre3-dev might be required. If
         you don't want its support, just configure SWIG without it by
         passing the --without-pcre option to its `configure' script.
 
 * To use shim-eth-vlan
    * vlan

Have fun,
Francesco
