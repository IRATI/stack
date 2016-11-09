## Data Center TCP-like policy

The goal of DCTCP is to achieve high burst tolerance, low latency, and high throughput, 
with commodity shallow buffered switches. DCTCP achieves these goals primarily by reacting 
to congestion in proportion to the extent of congestion. DCTCP requires active queue 
management scheme at switches.

Data Center TCP-like behavior in RINA can be achieved by altering RMT and DTCP policies.

### RMT DCTCP policy

The DCTCP policy for RMT is simple. It is possible to configure a queue size and a threshold. 
If queue size exceedes the threshold, the RMT starts to mark PDUs with explicit congestion 
flag (ECN). 

**Parameters that can be set:**
You can use the following parameters to alter policy behavior.

- `q_threshold:` Sets the queue threshold. If the queue size is exceeded, PDUs will be marked
with ECN flag.
- `q_max:` Maximal size of the queue. If exceeded, the PDU is dropped.

### DTCP DCTCP policy
The DCTCP policy for DTCP maintains an estimate of the fraction of packets that are marked 
with ECN flag. This value is updated with every PDU according the following formula:

`α ← (1−g) × α + g × F`

The DCTCP policy reacts to PDUs with ECN flag set by reducing the sender’s credit size using
the following formula:

`new_credit ← new_credit × (1−α/2)`

**Parameters that can be set:**
You can use the following parameter to alter policy behavior.

- `shift_g:` According the DCTCP paper, the g value should be small enough and all experiments 
in the paper use `g = 0.0625 (1/16)`. Thus, the `shift_g = 4` is `2^4 = 16`. 
