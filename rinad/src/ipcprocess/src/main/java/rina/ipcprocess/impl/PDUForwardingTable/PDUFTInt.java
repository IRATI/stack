package rina.ipcprocess.impl.PDUForwardingTable;

import rina.cdap.api.message.CDAPMessage;
import rina.ipcprocess.api.IPCProcess;
import rina.ipcprocess.impl.PDUForwardingTable.routingalgorithms.RoutingAlgorithmInt;
import rina.ipcprocess.impl.PDUForwardingTable.routingalgorithms.VertexInt;
import eu.irati.librina.ApplicationProcessNamingInformation;

public interface PDUFTInt {
	public void setIPCProcess(IPCProcess ipcProcess);
	
	public void setAlgorithm(RoutingAlgorithmInt routingAlgorithm, VertexInt sourceVertex);
	
	public void enrollmentToNeighbor(ApplicationProcessNamingInformation name, long address, boolean newMember, int portId);

	public boolean flowAllocated(long address, int portId, long neighborAddress, int neighborPortId);

	public boolean flowdeAllocated(int portId);

	public boolean propagateFSDB();

	public void updateAge();

	public boolean writeMessageRecieved(CDAPMessage objectsToModify, int srcPort);

	public void ForwardingTableupdate ();

}
