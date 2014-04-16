Branches
========

The IRATI stack repository contains 3 main branches: irati, wip and be:

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

This document describes the tests that have to succeed before integrating code
from one branch in the other.

Testing procedures
==================

On the following, we refer with the term 'testbeds' assuming that they are the
IRATI's vWall and EXPERIMENTA testbeds. Target platforms are Debian wheezy and
the 32/64 bits x86 based machines available on the testbeds.

'be' is merged into 'wip' when:
  * 'be' succeeds in the following tests without crashing on the project's
     testbeds:
         - An application can be run on top of the shim-dummy and shim-eth-vlan
         - Enrolment succeeds
         - An application can be run on top of a normal DIF (unreliable flow)
           on top of a shim
         - Forwarding is working
         - DIFs can be stacked
         - No major performance decreases are noticed

'wip' is merged into 'irati' when: 
  * 'wip' succeeds in the following tests without crashing on the project's
    testbeds.
         - An application can be run on top of the shim-dummy and shim-eth-vlan
         - Enrolment succeeds
         - An application can be run on top of a normal DIF (unreliable flow)
           on top of a shim
         - Forwarding is working
         - DIFs can be stacked
         - No major performance decreases are noticed

Tagging procedures
==================

A tag is stitched to 'be' when:
  * New functionalities are added

A tag is stitched to 'wip' when: 
  * 'be' is merged into 'wip'

A tag is stitched to irati when:
  * 'wip' is merged into 'irati'
