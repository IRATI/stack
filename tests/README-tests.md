The IRATI stack repository contains 3 main branches: irati, wip and be:
  * be (bleeding edge):     Highly unstable branch. The code can produce
                            warnings while compiling.
  * wip (work in progress): Unstable branch. The code may produce warnings
                            while compiling 
  * irati (main branch):    Stable branch. The code cannot produce warnings
                            while compiling.

This document describes the tests that have to succeed before integrating code
from one branch in the other.

On the following:
  * we refer with the term 'testbeds' assuming that they are the IRATI's vWall
    and EXPERIMENTA testbeds.
  * we refer to a tag with the following format: vMAJOR.MINOR.MICRO

A tag is stitched to 'be' when:
  * New functionalities are added

A tag is stitched to 'wip' when: 
  * 'be' succeeds in the following tests without crashing on the project's
    testbeds:
         - An application can be run on top of the shim-dummy and shim-eth-vlan
         - Enrolment succeeds
         - An application can be run on top of a normal DIF (unreliable flow)
           on top of a shim
         - Forwarding is working
         - DIFs can be stacked
         - No major performance decreases are noticed
   * 'be' is merged into 'wip'
  
A tag is stitched to irati when:
  * 'wip' succeeds in the following tests without crashing on the project's
    testbeds.
         - An application can be run on top of the shim-dummy and shim-eth-vlan
         - Enrolment succeeds
         - An application can be run on top of a normal DIF (unreliable flow)
           on top of a shim
         - Forwarding is working
         - DIFs can be stacked
         - No major performance decreases are noticed
  * 'wip' is merged into 'irati'
   