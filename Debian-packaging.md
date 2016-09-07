# Debian packaging

This document describes how to build Debian packages (*.deb) that provide RINA support on Debian machines.

Packaging the Linux kernel is a complex task so:
- we forked the official packaging, with minimal changes
- instructions to maintain this package are detailed below

Therefore, this repository contains a first `linux/debian` folder.

There are 3 other source packages for userland:
- `librina/debian`
- `rinad/debian`
- `rina-tools/debian`

## Linux kernel for Debian, with support for RINA

The differences with the official Debian packaging of the kernel are:

* No ABI dump. They're quite big and RINA patches always require an ABI bump, so we start with `rina.1` as abiname (see `debian/config/defines`) to avoid conflicts with official module packages. If needed, we could start checking ABI and bump.

* No installer file, except a dummy empty one. That would be a lot of useless files.

* 2 set of patches conflict with newer stable releases, since Debian stopped distributing 4.1.x versions for the kernel. Unless there's a specific need for them, they are dropped for the moment:

  - aufs (an alternative is overlayfs)
  - a few hardenings from grsecurity

### Updating to package to newer version of the kernel

The easiest place to find Debian tarballs is often http://snapshot.debian.org/package/linux/
They should be checked with `dscverify` (packages `devscripts` and `debian-keyring`).

To better track packaging changes from upstream, they should not be imported directly in the main branch:

1. checkout the commit of the last imported tarball, whose tree only contains a `linux/debian` folder (next steps are done from `linux/`)
2. delete the whole debian/ folder
3. extract the new tarball
4. add-remove with `git add debian`: `/.gitignore` and `/linux/debian/.gitignore` take care of excluding unwanted files, so make sure they apply, and only them
5. commit and mention which tarball you imported
6. remove or refresh patches that will conflict in a separate commit
7. merge into the main branch, which requires at least to solve a conflict in `debian/changelog`
8. start a new `debian/changelog` entry with `dch`

### Building from repository

From the `/linux` folder:

    # most of the time, you build snapshot packages
    # (distribution defaults to UNRELEASED but parameters are
    # forwarded to 'dch' so it can be overridden with -D option)
    debian/dch-snapshot

    # apply patches
    QUILT_PATCHES=$PWD/debian/patches QUILT_PC=.pc quilt push --quiltrc - -a -q --fuzz=0

    # prepare debian/
    debian/rules debian/rules.gen

In order to build librina, you need `linux-libc-dev`. On amd64, you can use the following commands:

    fakeroot make -f debian/rules.gen binary-arch_amd64_none binary-libc-dev_amd64

(consider passing a -j option to make)

### Compatibility

This package is for Debian Stable. However:
- it may build on Debian-derivatives (e.g. Ubuntu) of same age
- the built packages may be usable on newer versions of Debian (or Debian-derivatives)

## Userland packages

The steps to build the 3 source packages are the same, but they have different build dependencies. In particular, we have the following build-dependency graph:

    linux -> librina -> rinad -> rina-tools

What differs from a usual Debian source package is that we don't maintain Debian changelogs, mainly because we want to build snapshot packages, whose version is derived from current Git revision.

From any folder among `/librina`, `/rinad`, `/rina-tools`:

    # generate the debian/changelog file (distribution defaults to
    # `lsb_release -sc`, this can be overridden with a 'DIST=...' parameter)
    debian/rules

    # then do as you're used to do, for example:
    dpkg-buildpackage -b -uc # -uc to skip gpg signing

Not done:
- Java bindings
- ABI for libraries
