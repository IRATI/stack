package unitTest.PDUForwardingTable.fakeobjects;

import java.util.List;

import eu.irati.librina.ApplicationProcessNamingInformation;
import eu.irati.librina.DIFInformation;
import eu.irati.librina.Neighbor;
import rina.cdap.api.CDAPSessionManager;
import rina.delimiting.api.Delimiter;
import rina.encoding.api.Encoder;
import rina.enrollment.api.EnrollmentTask;
import rina.flowallocator.api.FlowAllocator;
import rina.ipcprocess.api.IPCProcess;
import rina.registrationmanager.api.RegistrationManager;
import rina.resourceallocator.api.ResourceAllocator;
import rina.ribdaemon.api.RIBDaemon;

public class FakeIPCProcess implements IPCProcess{
	
	CDAPSessionManager cdapSM = null;
	RIBDaemon ribd = null;
	Encoder encoder = null;

	public FakeIPCProcess(CDAPSessionManager cdapSM, RIBDaemon ribd, Encoder encoder)
	{
		this.cdapSM = cdapSM;
		this.ribd = ribd;
		this.encoder = encoder;
	}
	@Override
	public void execute(Runnable runnable) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public Delimiter getDelimiter() {
		// TODO Auto-generated method stub
		return null;
	}

	@Override
	public CDAPSessionManager getCDAPSessionManager() {
		return this.cdapSM;
	}

	@Override
	public Encoder getEncoder() {
		return this.encoder;
	}

	@Override
	public RIBDaemon getRIBDaemon() {
		return this.ribd;
	}

	@Override
	public EnrollmentTask getEnrollmentTask() {
		// TODO Auto-generated method stub
		return null;
	}

	@Override
	public ResourceAllocator getResourceAllocator() {
		return new FakeResourceAllocator();
	}

	@Override
	public RegistrationManager getRegistrationManager() {
		// TODO Auto-generated method stub
		return null;
	}

	@Override
	public FlowAllocator getFlowAllocator() {
		// TODO Auto-generated method stub
		return null;
	}

	@Override
	public DIFInformation getDIFInformation() {
		// TODO Auto-generated method stub
		return null;
	}

	@Override
	public void setDIFInformation(DIFInformation difInformation) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public Long getAddress() {
		// TODO Auto-generated method stub
		return (long) 1;
	}

	@Override
	public ApplicationProcessNamingInformation getName() {
		// TODO Auto-generated method stub
		return null;
	}

	@Override
	public State getOperationalState() {
		// TODO Auto-generated method stub
		return null;
	}

	@Override
	public void setOperationalState(State state) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public List<Neighbor> getNeighbors() {
		// TODO Auto-generated method stub
		return null;
	}
	@Override
	public long getAdressByname(ApplicationProcessNamingInformation arg0) {
		// TODO Auto-generated method stub
		return 0;
	}

}
