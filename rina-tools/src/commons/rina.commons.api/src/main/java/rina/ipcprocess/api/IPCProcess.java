package rina.ipcprocess.api;

import java.util.List;

import eu.irati.librina.ApplicationProcessNamingInformation;
import eu.irati.librina.DIFInformation;
import eu.irati.librina.Neighbor;
import rina.cdap.api.CDAPSessionManager;
import rina.delimiting.api.Delimiter;
import rina.encoding.api.Encoder;
import rina.enrollment.api.EnrollmentTask;
import rina.flowallocator.api.FlowAllocator;
import rina.registrationmanager.api.RegistrationManager;
import rina.resourceallocator.api.ResourceAllocator;
import rina.ribdaemon.api.RIBDaemon;

public interface IPCProcess {
	public static final String MANAGEMENT_AE = "Management";
    public static final String DATA_TRANSFER_AE = "Data Transfer";
    public static final int DEFAULT_MAX_SDU_SIZE_IN_BYTES = 10000;
	public static final long CONFIG_FILE_POLL_PERIOD_IN_MS = 5000;
	
	public enum State {NULL, INITIALIZED, ASSIGN_TO_DIF_IN_PROCESS, ASSIGNED_TO_DIF};
	
	public void execute(Runnable runnable);
	public Delimiter getDelimiter();
	public CDAPSessionManager getCDAPSessionManager();
	public Encoder getEncoder();
	public RIBDaemon getRIBDaemon();
	public EnrollmentTask getEnrollmentTask();
	public ResourceAllocator getResourceAllocator();
	public RegistrationManager getRegistrationManager() ;
	public FlowAllocator getFlowAllocator();
	public DIFInformation getDIFInformation();
	public void setDIFInformation(DIFInformation difInformation);
	public Long getAddress();
	public ApplicationProcessNamingInformation getName();
	public State getOperationalState();
	public void setOperationalState(State state);
	public List<Neighbor> getNeighbors();
	public long getAdressByname(ApplicationProcessNamingInformation name) throws Exception;
}
