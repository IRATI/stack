## Table of contents
````

1. Introduction
2. QTA Mux system overview
3. QTA Mux system detailed operation
    3.1 C/U mux
    3.2 Policers / Shapers
4. Example configuration
````

# 1. QTA (Quantitative Transport Agreement) Multiplexer (QTA Mux)

The QTA mux is a scheduling policy that facilitates trading 
the allocation of loss and delay between PDUs of different QoS 
classes that want to access the same N-1 output flow.

# 2. QTA Mux system overview

Each arriving PDU arriving at the RMT belongs to one of the QoS-Cubes supported
by the DIF. The QTAMux provides a mechanism to trade DeltaQ (allocation of 
loss and delay) between flows of different QoS-Cubes towards a particular N-1 port. The QTAMux
has three principal elements, as shown in the Figure below:

* **The Cherish-Urgency Multiplexer (C/U Mux)**: this trades DeltaQ (delay
and loss) between the (instantaneous) set of competing streams, using
loss to maintain schedulability.
* **The policer/shapers (P/S)**: these trade overall DeltaQ against loading
factor, performing intra-stream contention for traffic in the same
treatment class.
* **The stream queue**: this ensures in-sequence delivery while allowing the
delivered quality level to be varied dynamically.
Both the C/U Mux and the P/Ss perform matching between demand and
supply - they differ in the way that they express the trades within the
inherent two degrees of freedom. Note that the interaction of the QTA mux
with ECN marking (both the creation of CN marks and its response to them)
is for further study.

![Figure 1. Main components of the QTA Mux system](https://github.com/IRATI/stack/wiki/plugins-doc/qta-mux/qta-mux.png)
_Figure 1. Main components of the QTA Mux system_ 

# 3. QTA Mux system detailed operation

## 3.1 C/U mux
The Cherish-Urgency Multiplexer can be seen as performing inter-stream
contention. Its operation is relatively straightforward to describe. Each
incoming packet stream is assumed to have a pre-assigned cherish and
urgency levels, derived from the PDU associations: these correspond to
the stream's quality requirements in terms of loss and delay. Contention
between streams for access to the outgoing port is managed by admitting
packets based on their cherish classification level and servicing them
based on their urgency classification level. Thus, a packet's cherish level
determines its probability of loss while the urgency level determines
its probable delay. Since the cherish and urgency classifications are
independent of one another, the quality assigned to different streams
is not limited to a traditional priority ordering scheme; instead, a
two-dimensional classification is achieved, as illustrated in Figure 2.

![Figure 2. Two dimensional cherish-urgency classification](https://github.com/IRATI/stack/wiki/plugins-doc/qta-mux/cumux1.png)
_Figure 2. Two dimensional cherish-urgency classification_  

Admission control is realised by partitioning the buffer capacity of the C/U Mux and
associating each partition with a subset of the cherish levels, as shown in
Figure 3. A packet is admitted only if there is spare capacity in one of the partitions
associated with its cherish level, as shown in Figure 3. All partitions are
available to the most highly cherished packets, while packets having a lower
cherish level are limited to smaller subsets of partitions. This means that
highly cherished packets have a greater probability of finding available
buffer resources than those having a lower cherish level, and in turn
experience less loss.

![Figure 3. Partitioning of buffer space by cherish levels](https://github.com/IRATI/stack/wiki/plugins-doc/qta-mux/cumux2.png)
_Figure 3. Partitioning of buffer space by cherish levels_

For each possible Cherish level C, the C/U Mux is configured with an absolute 
threshold, a probabilistic one and a drop probability. 

Once in the Multiplexer, packets are serviced using a strict priority
queueing system according to their urgency level.

## 3.2 Policers / Shapers
Policer/Shapers can be seen as performing intra-stream contention for
traffic in the same treatment class. The operation of the C/U Mux delivers
a strict partial order between the service provided to the different classes,
but by itself allows traffic in classes higher in this partial order to dominate
those that are lower. In order to deliver bounded quality attenuation
to all classes, traffic in each class needs to satisfy certain preconditions
regarding both its rate and its short-term load pattern, which are aspects
of its QoS-cube. The first is regulated through policing and the second via
shaping. Policer/shapers enforce these preconditions, ensuring that traffic
within a class experiences contention within its allocated resource budget
before contending with the traffic of other classes. Policer/shapers provide
considerable flexibility in how this is done, for example, traffic that is out
of contract can be discarded or alternatively downgraded to a lower C/
U Mux class (the current implementation discards the traffic).

# 4. Example configuration
