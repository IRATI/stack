This document describes the tests that have to succeed 
before integrating code from one branch in the other.

Branches:
        * be    (bleeding edge, highly unstable)
        * wip   (work in progress, unstable)
        * irati (main branch, stable version)

A tag is stitched to 'be' when: 
  * New functionality is added

A tag is stitched to 'wip' when: 
  * 'be' succeeds in the following tests without crashing 
    on either the virtual wall or on EXPERIMENTA:
         - An application can be run on top of the shim-dummy and shim-eth-vlan
         - Enrolment succeeds
         - An application can be run on top of a normal DIF (unreliable flow) on top of a shim
         - Forwarding is working
         - DIFs can be stacked
         - No major performance decreases are noticed
   * 'be' is merged into 'wip'
  
A tag is stitched to irati when:
  * 'wip' succeeds in the following tests without crashing 
    on the virtual wall and on EXPERIMENTA:
         - An application can be run on top of the shim-dummy and shim-eth-vlan
         - Enrolment succeeds
         - An application can be run on top of a normal DIF (unreliable flow) on top of a shim
         - Forwarding is working
         - DIFs can be stacked
         - No major performance decreases are noticed
         - The stack can be run on different platforms (32-bit/64-bit, different Linux distros)
  * 'wip' is merged into 'irati'
   