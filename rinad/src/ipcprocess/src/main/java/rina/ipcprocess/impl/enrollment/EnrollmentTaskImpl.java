package rina.ipcprocess.impl.enrollment;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Timer;
import java.util.concurrent.ConcurrentHashMap;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import eu.irati.librina.ApplicationProcessNamingInformation;
import eu.irati.librina.FlowInformation;
import eu.irati.librina.FlowSpecification;
import eu.irati.librina.IPCException;
import eu.irati.librina.Neighbor;

import rina.cdap.api.CDAPSessionDescriptor;
import rina.cdap.api.CDAPSessionManager;
import rina.cdap.api.message.CDAPMessage;
import rina.encoding.api.Encoder;
import rina.enrollment.api.EnrollmentTask;
import rina.events.api.Event;
import rina.events.api.EventListener;
import rina.ipcprocess.impl.IPCProcess;
import rina.ipcprocess.impl.enrollment.ribobjects.AddressRIBObject;
import rina.ipcprocess.impl.enrollment.ribobjects.EnrollmentRIBObject;
import rina.ipcprocess.impl.enrollment.ribobjects.NeighborSetRIBObject;
import rina.ipcprocess.impl.enrollment.ribobjects.OperationalStatusRIBObject;
import rina.ipcprocess.impl.enrollment.ribobjects.WatchdogRIBObject;
import rina.ipcprocess.impl.enrollment.statemachines.BaseEnrollmentStateMachine;
import rina.ipcprocess.impl.enrollment.statemachines.BaseEnrollmentStateMachine.State;
import rina.ipcprocess.impl.enrollment.statemachines.EnrolleeStateMachine;
import rina.ipcprocess.impl.enrollment.statemachines.EnrollerStateMachine;
import rina.ipcprocess.impl.events.ConnectivityToNeighborLostEvent;
import rina.ipcprocess.impl.events.NMinusOneFlowAllocatedEvent;
import rina.ipcprocess.impl.events.NMinusOneFlowAllocationFailedEvent;
import rina.ipcprocess.impl.events.NMinusOneFlowDeallocatedEvent;
import rina.resourceallocator.api.ResourceAllocator;
import rina.ribdaemon.api.RIBDaemon;
import rina.ribdaemon.api.RIBDaemonException;
import rina.ribdaemon.api.RIBObject;

/**
 * Current limitations: Addresses of IPC processes are allocated forever (until we lose the connection with them)
 * @author eduardgrasa
 *
 */
public class EnrollmentTaskImpl implements EnrollmentTask, EventListener{
	
	public static final long DEFAULT_ENROLLMENT_TIMEOUT_IN_MS = 10*1000;
	
	private static final Log log = LogFactory.getLog(EnrollmentTaskImpl.class);
	
	/**
	 * Stores the enrollee state machines, one per remote IPC process that this IPC 
	 * process is enrolled to.
	 */
	private Map<String, BaseEnrollmentStateMachine> enrollmentStateMachines = null;
	
	/**
	 * The maximum time to wait between steps of the enrollment sequence (in ms)
	 */
	private long timeout = 0;
	
	/**
	 * The runnable that will try to enroll us to known neighbors that we're not 
	 * currently enrolled with
	 */
	private NeighborsEnroller neighborsEnroller = null;
	
	private RIBDaemon ribDaemon = null;
	private ResourceAllocator resourceAllocator = null;
	private CDAPSessionManager cdapSessionManager = null;
	private Encoder encoder = null;
	private Map<Long, Neighbor> portIdsPendingToBeAllocated = null;
	private IPCProcess ipcProcess = null;
	
	private Timer timer = null;

	public EnrollmentTaskImpl(){
		this.enrollmentStateMachines = new ConcurrentHashMap<String, BaseEnrollmentStateMachine>();
		this.timeout = DEFAULT_ENROLLMENT_TIMEOUT_IN_MS;
		this.portIdsPendingToBeAllocated = new ConcurrentHashMap<Long, Neighbor>();
		this.timer = new Timer();
	}
	
	public void setIPCProcess(IPCProcess ipcProcess){
		this.ipcProcess = ipcProcess;
		this.ribDaemon = ipcProcess.getRIBDaemon();
		this.resourceAllocator = ipcProcess.getResourceAllocator();
		this.cdapSessionManager = ipcProcess.getCDAPSessionManager();
		this.encoder = ipcProcess.getEncoder();
		populateRIB();
		subscribeToEvents();
		this.neighborsEnroller = new NeighborsEnroller(this);
		ipcProcess.execute(this.neighborsEnroller);
	}
	
	/**
	 * Subscribe to all M_CONNECTs, M_CONNECT_R, M_RELEASE and M_RELEASE_R
	 */
	private void populateRIB(){
		try{
			RIBObject ribObject = new NeighborSetRIBObject();
			this.ribDaemon.addRIBObject(ribObject);
			ribObject = new EnrollmentRIBObject(this);
			this.ribDaemon.addRIBObject(ribObject);
			ribObject = new OperationalStatusRIBObject(this);
			this.ribDaemon.addRIBObject(ribObject);
			ribObject = new AddressRIBObject();
			this.ribDaemon.addRIBObject(ribObject);
			ribObject = new WatchdogRIBObject();
			this.ribDaemon.addRIBObject(ribObject);
		}catch(RIBDaemonException ex){
			ex.printStackTrace();
			log.error("Could not subscribe to RIB Daemon:" +ex.getMessage());
		}
	}
	
	private void subscribeToEvents(){
		this.ribDaemon.subscribeToEvent(Event.N_MINUS_1_FLOW_DEALLOCATED, this);
		this.ribDaemon.subscribeToEvent(Event.N_MINUS_1_FLOW_ALLOCATED, this);
		this.ribDaemon.subscribeToEvent(Event.N_MINUS_1_FLOW_ALLOCATION_FAILED, this);
	}
	
	public RIBDaemon getRIBDaemon(){
		return this.ribDaemon;
	}
	
	/**
	 * Returns the enrollment state machine associated to the cdap descriptor.
	 * @param cdapSessionDescriptor
	 * @return
	 */
	private BaseEnrollmentStateMachine getEnrollmentStateMachine(CDAPSessionDescriptor cdapSessionDescriptor, boolean remove) throws Exception{
		try{
			String myApName = ipcProcess.getName().getProcessName();
			String sourceAPName = cdapSessionDescriptor.getSourceApplicationProcessNamingInfo().getProcessName();
			String destAPName = cdapSessionDescriptor.getDestinationApplicationProcessNamingInfo().getProcessName();
			
			if (myApName.equals(sourceAPName)){
				return getEnrollmentStateMachine(destAPName, cdapSessionDescriptor.getPortId(), remove);
			}else{
				throw new Exception("This IPC process is not the intended recipient of the CDAP message");
			}
		}catch(RIBDaemonException ex){
			log.error(ex);
			return null;
		}
	}
	
	/**
	 * Gets and/or removes an existing enrollment state machine from the list of enrollment state machines
	 * @param apName
	 * @param remove
	 * @return
	 */
	public synchronized BaseEnrollmentStateMachine getEnrollmentStateMachine(String apName, int portId, boolean remove){
		if (remove){
			log.debug("Removing enrollment state machine associated to "+apName+" "+portId);
			return enrollmentStateMachines.remove(apName+"-"+portId);
		}else{
			return enrollmentStateMachines.get(apName+"-"+portId);
		}
	}
	
	/**
	 * Finds out if the ICP process is already enrolled to the IPC process identified by 
	 * the provided apNamingInfo
	 * @param apNamingInfo
	 * @return
	 */
	public synchronized boolean isEnrolledTo(String applicationProcessName){
		Iterator<Entry<String, BaseEnrollmentStateMachine>> iterator = enrollmentStateMachines.entrySet().iterator();
		Entry<String, BaseEnrollmentStateMachine> currentEntry = null;
		
		while(iterator.hasNext()){
			currentEntry = iterator.next();
			if (currentEntry.getValue().getRemotePeerNamingInfo().getProcessName().equals(applicationProcessName)){
				if (currentEntry.getValue().getState() == State.ENROLLED){
					return true;
				}
			}
		}

		return false;
	}
	
	/**
	 * Return the list of IPC Process names we're currently enrolled to
	 * @return
	 */
	public List<String> getEnrolledIPCProcessNames(){
		Iterator<Entry<String, BaseEnrollmentStateMachine>> iterator = null;
		List<String> result = new ArrayList<String>();

		synchronized(this){
			iterator = enrollmentStateMachines.entrySet().iterator();
		}

		while(iterator.hasNext()){
			result.add(iterator.next().getValue().getRemotePeerNamingInfo().getProcessName());
		}

		return result;
	}
	
	/**
	 * Creates an enrollment state machine with the remote IPC process identified by the apNamingInfo
	 * @param apNamingInfo
	 * @param enrollee true if this IPC process is the one that initiated the 
	 * enrollment sequence (i.e. it is the application process that wants to 
	 * join the DIF)
	 * @return
	 */
	private BaseEnrollmentStateMachine createEnrollmentStateMachine(
			ApplicationProcessNamingInformation apNamingInfo, int portId, boolean enrollee) throws Exception{
		BaseEnrollmentStateMachine enrollmentStateMachine = null;

		if (apNamingInfo.getEntityName() == null || 
				apNamingInfo.getEntityName().equals(IPCProcess.MANAGEMENT_AE)){
			if (enrollee){
				enrollmentStateMachine = new EnrolleeStateMachine(ribDaemon, cdapSessionManager, encoder, 
						apNamingInfo, this, timeout);
			}else{
				enrollmentStateMachine = new EnrollerStateMachine(ribDaemon, cdapSessionManager, encoder, 
						apNamingInfo, this, timeout);
			}

			synchronized(this){
				enrollmentStateMachines.put(apNamingInfo.getProcessName()+"-"+portId, 
						enrollmentStateMachine);
			}

			log.debug("Created a new Enrollment state machine for remote IPC process: " + 
					apNamingInfo.getProcessNamePlusInstance());
			return enrollmentStateMachine;
		}

		throw new Exception("Unknown application entity for enrollment: "+apNamingInfo.getEntityName());
	}
	
	/**
	 * Starts the enrollment program
	 * @param cdapMessage
	 * @param cdapSessionDescriptor
	 */
	public void initiateEnrollment(Neighbor candidate){
		if (this.isEnrolledTo(candidate.getName().getProcessName())){
			String message = "Already enrolled to IPC Process "+candidate.getName().getProcessNamePlusInstance();
			log.error(message);
			return;
		}

		//Request the allocation of a new N-1 Flow to the destination IPC Process, dedicated to layer management
		//FIXME not providing FlowSpec information
		ApplicationProcessNamingInformation candidateNamingInfo = candidate.getName();
		candidateNamingInfo.setEntityName(IPCProcess.MANAGEMENT_AE);
		ApplicationProcessNamingInformation apNamingInfo = 
				new ApplicationProcessNamingInformation(ipcProcess.getName().getProcessName(), 
						ipcProcess.getName().getProcessInstance());
		apNamingInfo.setEntityName(IPCProcess.MANAGEMENT_AE);
		FlowInformation flowInformation = new FlowInformation();
		flowInformation.setRemoteAppName(candidate.getName());
		flowInformation.setLocalAppName(candidateNamingInfo);
		flowInformation.setFlowSpecification(new FlowSpecification());
		flowInformation.setDifName(candidate.getSupportingDifName());
		long handle = -1;
		try{
			handle = this.resourceAllocator.getNMinus1FlowManager().allocateNMinus1Flow(flowInformation);
		} catch(IPCException ex) {
			log.error("Problems allocating flow through N-1 DIF: "+ex.getMessage());
			return;
		}

		//Store state of pending flows
		this.portIdsPendingToBeAllocated.put(handle, candidate);
	}

	/**
	 * Called by the RIB Daemon when an M_CONNECT message is received
	 * @param CDAPMessage the cdap message received
	 * @param CDAPSessionDescriptor contains the data about the CDAP session (including the portId)
	 */
	public void connect(CDAPMessage cdapMessage, CDAPSessionDescriptor cdapSessionDescriptor) {
		log.debug("Received M_CONNECT cdapMessage from portId "+cdapSessionDescriptor.getPortId());
		
		//1 Find out if the sender is really connecting to us
		if(!cdapMessage.getDestApName().equals(ipcProcess.getName().getProcessName())){
			//Ignore
			log.warn("Received an M_CONNECT message whose destination was not this IPC Process, ignoring it. "+cdapMessage.toString());
			return;
		}

		//2 Find out if we are already enrolled to the remote IPC process
		if (this.isEnrolledTo(cdapSessionDescriptor.getDestinationApplicationProcessNamingInfo().getProcessName())){
			try{
				String message = "Received an enrollment request for an IPC process I'm already enrolled to";
				log.error(message);
				int portId = cdapSessionDescriptor.getPortId();
				CDAPMessage errorMessage = 
						cdapSessionManager.getOpenConnectionResponseMessage(portId, cdapMessage.getAuthMech(), null, 
								cdapMessage.getSrcAEInst(), cdapMessage.getSrcAEName(), 
								cdapMessage.getSrcApInst(), cdapMessage.getSrcApName(), -2, message, 
								null, cdapMessage.getDestAEName(), cdapMessage.getDestApInst(), 
								cdapMessage.getDestApName(), cdapMessage.getInvokeID());
				sendErrorMessageAndDeallocateFlow(errorMessage, portId);
			}catch(Exception e){
				log.error(e);
			}
			
			return;
		}
		
		//3 Initiate the enrollment
		try{

			EnrollerStateMachine enrollmentStateMachine = (EnrollerStateMachine) this.createEnrollmentStateMachine(
					cdapSessionDescriptor.getDestinationApplicationProcessNamingInfo(), 
					cdapSessionDescriptor.getPortId(), false);
			enrollmentStateMachine.connect(cdapMessage, cdapSessionDescriptor.getPortId());
		}catch(Exception ex){
			log.error(ex.getMessage());
			try{
				int portId = cdapSessionDescriptor.getPortId();
				CDAPMessage errorMessage =
					cdapSessionManager.getOpenConnectionResponseMessage(portId, cdapMessage.getAuthMech(), null, cdapMessage.getSrcAEInst(), cdapMessage.getSrcAEName(), 
							cdapMessage.getSrcApInst(), cdapMessage.getSrcApName(), -2, ex.getMessage(), null, cdapMessage.getDestAEName(), cdapMessage.getDestApInst(), 
							cdapMessage.getDestApName(), cdapMessage.getInvokeID());
				sendErrorMessageAndDeallocateFlow(errorMessage, portId);
			}catch(Exception e){
				log.error(e);
			}
		}
	}

	/**
	 * Called by the RIB Daemon when an M_CONNECT_R message is received
	 * @param CDAPMessage the cdap message received
	 * @param CDAPSessionDescriptor contains the data about the CDAP session (including the portId)
	 */
	public void connectResponse(CDAPMessage cdapMessage, CDAPSessionDescriptor cdapSessionDescriptor) {
		log.debug("Received M_CONNECT_R cdapMessage from portId "+cdapSessionDescriptor.getPortId());

		try{
			EnrolleeStateMachine enrollmentStateMachine = (EnrolleeStateMachine) this.getEnrollmentStateMachine(cdapSessionDescriptor, false);
			enrollmentStateMachine.connectResponse(cdapMessage, cdapSessionDescriptor);
		}catch(Exception ex){
			//Error getting the enrollment state machine
			log.error(ex.getMessage());
			try{
				int portId = cdapSessionDescriptor.getPortId();
				ribDaemon.sendMessage(cdapSessionManager.getReleaseConnectionRequestMessage(portId, null, false), portId, null);
			}catch(Exception e){
				log.error(e);
			}
		}
	}

	/**
	 * Called by the RIB Daemon when an M_RELEASE message is received
	 * @param CDAPMessage the cdap message received
	 * @param CDAPSessionDescriptor contains the data about the CDAP session (including the portId)
	 */
	public void release(CDAPMessage cdapMessage, CDAPSessionDescriptor cdapSessionDescriptor){
		log.debug("Received M_RELEASE cdapMessage from portId "+cdapSessionDescriptor.getPortId());

		try{
			BaseEnrollmentStateMachine enrollmentStateMachine = this.getEnrollmentStateMachine(cdapSessionDescriptor, true);
			enrollmentStateMachine.release(cdapMessage, cdapSessionDescriptor);
		}catch(Exception ex){
			//Error getting the enrollment state machine
			log.error(ex.getMessage());
			try{
				int portId = cdapSessionDescriptor.getPortId();
				ribDaemon.sendMessage(cdapSessionManager.getReleaseConnectionRequestMessage(portId, null, false), portId, null);
			}catch(Exception e){
				log.error(e);
			}
		}
	}

	/**
	 * Called by the RIB Daemon when an M_RELEASE_R message is received
	 * @param CDAPMessage the cdap message received
	 * @param CDAPSessionDescriptor contains the data about the CDAP session (including the portId)
	 */
	public void releaseResponse(CDAPMessage cdapMessage, CDAPSessionDescriptor cdapSessionDescriptor){
		log.debug("Received M_RELEASE_R cdapMessage from portId "+cdapSessionDescriptor.getPortId());

		try{
			BaseEnrollmentStateMachine enrollmentStateMachine = this.getEnrollmentStateMachine(cdapSessionDescriptor, true);
			enrollmentStateMachine.releaseResponse(cdapMessage, cdapSessionDescriptor);
		}catch(Exception ex){
			//Error getting the enrollment state machine
			log.error(ex.getMessage());
			try{
				int portId = cdapSessionDescriptor.getPortId();
				ribDaemon.sendMessage(cdapSessionManager.getReleaseConnectionRequestMessage(portId, null, false), portId, null);
			}catch(Exception e){
				log.error(e);
			}
		}
	}
	
	/**
	 * Called when the events we're subscribed to happen
	 */
	public void eventHappened(Event event) {
		if (event.getId().equals(Event.N_MINUS_1_FLOW_DEALLOCATED)){
			NMinusOneFlowDeallocatedEvent flowEvent = (NMinusOneFlowDeallocatedEvent) event;
			this.nMinusOneFlowDeallocated(flowEvent.getPortId(), flowEvent.getCdapSessionDescriptor());
		}else if (event.getId().equals(Event.N_MINUS_1_FLOW_ALLOCATED)){
			NMinusOneFlowAllocatedEvent flowEvent = (NMinusOneFlowAllocatedEvent) event;
			this.nMinusOneFlowAllocated(flowEvent);
		}else if (event.getId().equals(Event.N_MINUS_1_FLOW_ALLOCATION_FAILED)){
			NMinusOneFlowAllocationFailedEvent flowEvent = (NMinusOneFlowAllocationFailedEvent) event;
			this.nMinusOneFlowAllocationFailed(flowEvent);
		}
	}
	
	/**
	 * Called by the RIB Daemon when the flow supporting the CDAP session with the remote peer
	 * has been deallocated
	 * @param cdapSessionDescriptor
	 */
	private void nMinusOneFlowDeallocated(int portId, CDAPSessionDescriptor cdapSessionDescriptor){
		//1 Check if the flow deallocated was a management flow
		if(cdapSessionDescriptor == null){
			return;
		}
		
		//1 Remove the enrollment state machine from the list
		try{
			BaseEnrollmentStateMachine enrollmentStateMachine = this.getEnrollmentStateMachine(cdapSessionDescriptor, true);
			if (enrollmentStateMachine == null){
				//Do nothing, we had already cleaned up
				return;
			}else{
				enrollmentStateMachine.flowDeallocated(cdapSessionDescriptor);
			}
		}catch(Exception ex){
			log.error(ex);
		}
		
		//3 Check if we still have connectivity to the neighbor, if not, issue a ConnectivityLostEvent
		Iterator<String> iterator = this.enrollmentStateMachines.keySet().iterator();
		while(iterator.hasNext()){
			if (iterator.next().startsWith(cdapSessionDescriptor.getDestApName())){
				//We still have connectivity with the neighbor, return
				return;
			}
		}
		
		//We don't have connectivity to the neighbor, issue a Connectivity lost event
		List<Neighbor> neighbors = ipcProcess.getNeighbors();
		for(int i=0; i<neighbors.size(); i++){
			if(neighbors.get(i).getName().getProcessName().equals(cdapSessionDescriptor.getDestApName())){
				ConnectivityToNeighborLostEvent event2 = new ConnectivityToNeighborLostEvent(neighbors.get(i));
				log.debug("Notifying the Event Manager about a new event.");
				log.debug(event2.toString());
				this.ribDaemon.deliverEvent(event2);
				return;
			}
		}
	}
	
	/**
	 * Called when a new N-1 flow has been allocated
	 * @param portId
	 */
	private void nMinusOneFlowAllocated(NMinusOneFlowAllocatedEvent flowEvent){
		Neighbor neighbor = this.portIdsPendingToBeAllocated.remove(flowEvent.getHandle());
		if (neighbor == null){
			return;
		}
		
		EnrolleeStateMachine enrollmentStateMachine = null;
		
		//1 Tell the enrollment task to create a new Enrollment state machine
		try{
			enrollmentStateMachine = (EnrolleeStateMachine) this.createEnrollmentStateMachine(
					neighbor.getName(), flowEvent.getFlowInformation().getPortId(), true);
		}catch(Exception ex){
			//Should never happen, fix it!
			log.error(ex);
			return;
		}
		
		//2 Tell the enrollment state machine to initiate the enrollment (will require an M_CONNECT message and a port Id)
		try{
			enrollmentStateMachine.initiateEnrollment(neighbor, flowEvent.getFlowInformation().getPortId());
		}catch(IPCException ex){
			log.error(ex);
		}
	}
	
	/**
	 * Called when a new N-1 flow allocation has failed
	 * @param portId
	 * @param flowService
	 * @param resultReason
	 */
	private void nMinusOneFlowAllocationFailed(NMinusOneFlowAllocationFailedEvent event){
		Neighbor neighbor = this.portIdsPendingToBeAllocated.remove(event.getHandle());
		if (neighbor == null){
			return;
		}
		
		log.warn("The allocation of management flow identified by handle "+event.getHandle()
				+" with the following charachteristics "+event.getFlowInformation().toString()
				+" has failed. Error code: "+event.getFlowInformation().getPortId());
		
		//TODO inform the one that triggered the enrollment?
	}
	
	/**
	 * Called by the enrollment state machine when the enrollment sequence fails
	 * @param remotePeer
	 * @param portId
	 * @param enrollee
	 * @param sendMessage
	 * @param reason
	 */
	public void enrollmentFailed(ApplicationProcessNamingInformation remotePeerNamingInfo, int portId, 
			String reason, boolean enrollee, boolean sendReleaseMessage){
		log.error("An error happened during enrollment of remote IPC Process "+ 
				remotePeerNamingInfo.getProcessNamePlusInstance()+ " because of " +reason+". Aborting the operation");
		//1 Remove enrollment state machine from the store
		this.getEnrollmentStateMachine(remotePeerNamingInfo.getProcessName(), portId, true);

		//2 Send message and deallocate flow if required
		if(sendReleaseMessage){
			try{
				CDAPMessage errorMessage = cdapSessionManager.getReleaseConnectionRequestMessage(portId, null, false);
				sendErrorMessageAndDeallocateFlow(errorMessage, portId);
			}catch(Exception ex){
				log.error(ex);
			}
		}
	}
	 
	 /**
	  * Called by the enrollment state machine when the enrollment request has been completed successfully
	  * @param candidate the IPC process we were trying to enroll to
	  * @param enrollee true if this IPC process is the one that initiated the 
	  * enrollment sequence (i.e. it is the application process that wants to 
	  * join the DIF)
	  */
	 public void enrollmentCompleted(Neighbor dafMember, boolean enrollee){
		 //TODO check what to do with this now
		/* if (enrollee){
			 //request the allocation of N-1 flows to the neighbor's Data Transfer AE
			 RequestNMinusOneFlowAllocation task = new RequestNMinusOneFlowAllocation(dafMember, 
					 ipcProcess.get, this.resourceAllocator.getNMinus1FlowManager(), 
					 RINAConfiguration.getInstance().getDIFConfiguration(this.getIPCProcess().getDIFName()));
			 timer.schedule(task, 200);
		 }*/
	 }
	
	/**
	 * Sends the CDAP Message and calls the RMT to deallocate the flow identified by portId
	 * @param cdapMessage
	 * @param portId
	 */
	private void sendErrorMessageAndDeallocateFlow(CDAPMessage cdapMessage, int portId){
		try{
			ribDaemon.sendMessage(cdapMessage, portId, null);
		}catch(Exception ex){
			log.error(ex);
		}
		
		try{
			this.resourceAllocator.getNMinus1FlowManager().deallocateNMinus1Flow(portId);
		}catch(Exception ex){
			log.error(ex.getMessage());
		}
	}
}
