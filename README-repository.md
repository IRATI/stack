The IRATI repository
====================

The IRATI repository is located at https://github.com/dana-i2cat/irati

NOTE: In order to avoid problems with the repository, please clone it on a 
      case sensitive file-system.

Branches
========

The repository contains 3 main branches: irati, wip and be:

  * be (bleeding edge):
    This branch is the highly unstable branch. The code there can produce
    warnings while compiling. Compilation and installation can be broken or
    could require tweaks from time to time.

  * wip (work in progress):
    This is the unstable branch. The code may produce warnings while
    compiling. Compilation must not be broken, installation might require
    minor tweaks in order to complete successfully.

  * irati (main branch):
    This is the stable branch. The code cannot produce warnings while
    compiling and must complete successfully on the target platforms.
    Installation cannot require tweaks.

Tags
====

A tag is stitched to 'be' when:
  * New functionalities are added

A tag is stitched to 'wip' when:
  * 'be' is merged into 'wip'

A tag is stitched to irati when:
  * 'wip' is merged into 'irati'

Testing
=======

Refer to tests/README-tests.md for testing details
