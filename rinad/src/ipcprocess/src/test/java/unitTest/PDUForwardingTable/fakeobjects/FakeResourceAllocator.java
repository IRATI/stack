package unitTest.PDUForwardingTable.fakeobjects;

import rina.ipcprocess.api.IPCProcess;
import rina.resourceallocator.api.NMinus1FlowManager;
import rina.resourceallocator.api.PDUForwardingTable;
import rina.resourceallocator.api.ResourceAllocator;

public class FakeResourceAllocator implements ResourceAllocator{

	@Override
	public void setIPCProcess(IPCProcess arg0) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public NMinus1FlowManager getNMinus1FlowManager() {
		return new FakeNMinus1FlowManager();
	}

	@Override
	public PDUForwardingTable getPDUForwardingTable() {
		// TODO Auto-generated method stub
		return null;
	}
	

}
