package rina.pduftg.api;

import eu.irati.librina.PDUFTableGeneratorConfiguration;
import rina.ipcprocess.api.IPCProcess;

public interface PDUFTGeneratorPolicy {

	public void setIPCProcess(IPCProcess ipcProcess);
	
	public void setDIFConfiguration(PDUFTableGeneratorConfiguration pduFTGConfig);
	
	public void enrollmentToNeighbor(long address, boolean newMember, int portId);

	public void flowAllocated(long address, int portId, long neighborAddress, int neighborPortId);

	public boolean flowDeallocated(int portId);
}
