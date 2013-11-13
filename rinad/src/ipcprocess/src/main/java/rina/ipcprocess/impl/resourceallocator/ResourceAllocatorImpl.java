package rina.resourceallocator.impl;

import rina.ipcprocess.api.IPCProcess;
import rina.ipcprocess.impl.resourceallocator.flowmanager.NMinus1FlowManagerImpl;
import rina.resourceallocator.api.BaseResourceAllocator;
import rina.resourceallocator.api.NMinus1FlowManager;
import rina.resourceallocator.api.PDUForwardingTable;
import rina.resourceallocator.impl.pduforwarding.PDUForwardingTableImpl;

/**
 * The Resource Allocator (RA) is the core of management in the IPC Process. 
 * The degree of decentralization depends on the policies and how it is used. The RA has a set of meters 
 * and dials that it can manipulate. The meters fall in 3 categories:
 * 		Ð Traffic characteristics from the user of the DIF
 * 		Ð Traffic characteristics of incoming and outgoing flows
 * 		Ð Information from other members of the DIF
 * The Dials:
 * 		Ð Creation/Deletion of QoS Classes
 * 		Ð Data Transfer QoS Sets
 *		Ð Modifying Data Transfer Policy Parameters
 * 		Ð Creation/Deletion of RMT Queues
 * 		Ð Modify RMT Queue Servicing
 * 		Ð Creation/Deletion of (N-1)-flows
 * 		Ð Assignment of RMT Queues to (N-1)-flows
 * 		Ð Forwarding Table Generator Output
 * 
 * @author eduardgrasa
 *
 */
public class ResourceAllocatorImpl extends BaseResourceAllocator{
	
	private NMinus1FlowManagerImpl nMinus1FlowManager = null;
	
	private PDUForwardingTableImpl pduForwardingTable = null;

	public ResourceAllocatorImpl(){
		pduForwardingTable = new PDUForwardingTableImpl();
		nMinus1FlowManager = new NMinus1FlowManagerImpl(pduForwardingTable);
	}
	
	public NMinus1FlowManager getNMinus1FlowManager() {
		return this.nMinus1FlowManager;
	}
	
	public PDUForwardingTable getPDUForwardingTable() {
		return pduForwardingTable;
	}
	
	public void setIPCProcess(IPCProcess ipcProcess){
		super.setIPCProcess(ipcProcess);
		this.nMinus1FlowManager.setIPCProcess(ipcProcess);
		this.pduForwardingTable.setIPCProcess(ipcProcess);
	}
}
