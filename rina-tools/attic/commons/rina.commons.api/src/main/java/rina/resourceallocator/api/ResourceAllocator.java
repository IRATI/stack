package rina.resourceallocator.api;

import rina.ipcprocess.api.IPCProcessComponent;

/**
 * The Resource Allocator (RA) is the core of management in the IPC Process. 
 * The degree of decentralization depends on the policies and how it is used. The RA has a set of meters 
 * and dials that it can manipulate. The meters fall in 3 categories:
 * 	Traffic characteristics from the user of the DIF
 * 	Traffic characteristics of incoming and outgoing flows
 * 	Information from other members of the DIF
 * The Dials:
 * 	Creation/Deletion of QoS Classes
 * 	Data Transfer QoS Sets
 *	Modifying Data Transfer Policy Parameters
 * 	Creation/Deletion of RMT Queues
 * 	Modify RMT Queue Servicing
 * 	Creation/Deletion of (N-1)-flows
 * 	Assignment of RMT Queues to (N-1)-flows
 * 	Forwarding Table Generator Output
 * 
 * @author eduardgrasa
 *
 */
public interface ResourceAllocator extends IPCProcessComponent {

	/**
	 * Returns the N-1 Flow Manager of this IPC Process
	 * @return
	 */
	public NMinus1FlowManager getNMinus1FlowManager();
	
	/**
	 * Returns the PDU Forwarding table of this IPC Process
	 * @return
	 */
	public PDUForwardingTable getPDUForwardingTable();

}
