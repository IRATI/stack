package unitTest.PDUForwardingTable.fakeobjects;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import eu.irati.librina.AllocateFlowRequestResultEvent;
import eu.irati.librina.ApplicationProcessNamingInformation;
import eu.irati.librina.DeallocateFlowResponseEvent;
import eu.irati.librina.FlowDeallocatedEvent;
import eu.irati.librina.FlowInformation;
import eu.irati.librina.FlowRequestEvent;
import eu.irati.librina.IPCException;
import eu.irati.librina.IPCProcessDIFRegistrationEvent;
import rina.ipcprocess.api.IPCProcess;
import rina.ipcprocess.impl.PDUForwardingTable.PDUFTImpl;
import rina.resourceallocator.api.NMinus1FlowManager;

public class FakeNMinus1FlowManager implements NMinus1FlowManager{
	private static final Log log = LogFactory.getLog(FakeNMinus1FlowManager.class);
	@Override
	public void setIPCProcess(IPCProcess arg0) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public long allocateNMinus1Flow(FlowInformation arg0) throws IPCException {
		// TODO Auto-generated method stub
		return 0;
	}

	@Override
	public void allocateRequestResult(AllocateFlowRequestResultEvent arg0)
			throws IPCException {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void deallocateFlowResponse(DeallocateFlowResponseEvent arg0)
			throws IPCException {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void deallocateNMinus1Flow(int arg0) throws IPCException {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void flowAllocationRequested(FlowRequestEvent arg0)
			throws IPCException {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void flowDeallocatedRemotely(FlowDeallocatedEvent arg0)
			throws IPCException {
		// TODO Auto-generated method stub
		
	}

	@Override
	public FlowInformation[] getAllNMinus1FlowsInformation() {
		FlowInformation f1 = new FlowInformation();
		f1.setPortId(1);
		FlowInformation[] flowsI = {f1};
		log.debug("hola");
		return flowsI;
	}

	@Override
	public FlowInformation getNMinus1FlowInformation(int arg0)
			throws IPCException {
		FlowInformation f1 = new FlowInformation();
		f1.setPortId(1);
		return f1;
	}

	@Override
	public void processRegistrationNotification(
			IPCProcessDIFRegistrationEvent arg0) throws IPCException {
		// TODO Auto-generated method stub
		
	}

	@Override
	public boolean isSupportingDIF(ApplicationProcessNamingInformation arg0) {
		// TODO Auto-generated method stub
		return false;
	}

}
