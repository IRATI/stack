package rina.ipcprocess.impl.enrollment.statemachines;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import rina.cdap.api.CDAPSessionDescriptor;
import rina.cdap.api.message.CDAPMessage;
import rina.cdap.api.message.CDAPMessage.AuthTypes;
import rina.cdap.api.message.ObjectValue;
import rina.enrollment.api.EnrollmentInformationRequest;
import rina.enrollment.api.EnrollmentRequest;
import rina.ipcprocess.api.IPCProcess;
import rina.ipcprocess.impl.ecfp.DataTransferConstantsRIBObject;
import rina.ipcprocess.impl.enrollment.ribobjects.AddressRIBObject;
import rina.ipcprocess.impl.enrollment.ribobjects.NeighborSetRIBObject;
import rina.ipcprocess.impl.flowallocator.ribobjects.QoSCubeSetRIBObject;
import rina.ipcprocess.impl.registrationmanager.ribobjects.DirectoryForwardingTableEntrySetRIBObject;
import rina.ribdaemon.api.RIBDaemonException;
import rina.ribdaemon.api.RIBObjectNames;
import eu.irati.librina.ApplicationProcessNamingInformation;
import eu.irati.librina.ApplicationRegistrationVector;
import eu.irati.librina.DIFInformation;
import eu.irati.librina.DataTransferConstants;
import eu.irati.librina.IPCException;
import eu.irati.librina.Neighbor;
import eu.irati.librina.NeighborList;
import eu.irati.librina.QoSCube;
import eu.irati.librina.rina;

/**
 * The state machine of the party that wants to 
 * become a new member of the DIF.
 * @author eduardgrasa
 *
 */
public class EnrolleeStateMachine extends BaseEnrollmentStateMachine{

	private static final Log log = LogFactory.getLog(EnrolleeStateMachine.class);
	
	private IPCProcess ipcProcess = null;
	
	/**
	 * True if early start is allowed by the enrolling IPC Process
	 */
	private boolean allowedToStart = false;
	
	private CDAPMessage stopEnrollmentRequestMessage = null;
	
	/** The DIF Information */
	private DIFInformation difInformation = null;
	
	/** The enrollment request, issued by the IPC Manager */
	private EnrollmentRequest enrollmentRequest = null;
	
	private boolean wasDIFMemberBeforeEnrollment = false;
	
	public EnrolleeStateMachine(IPCProcess ipcProcess, 
			ApplicationProcessNamingInformation remoteNamingInfo, long timeout){
		super(ipcProcess.getRIBDaemon(), ipcProcess.getCDAPSessionManager(), ipcProcess.getEncoder(), 
				remoteNamingInfo, ipcProcess.getEnrollmentTask(), timeout, null);
		this.ipcProcess = ipcProcess;
		difInformation = ipcProcess.getDIFInformation();
	}
	
	public EnrollmentRequest getEnrollmentRequest() {
		return enrollmentRequest;
	}
	
	/**
	 * Called by the DIFMembersSetObject to initiate the enrollment sequence 
	 * with a remote IPC Process
	 * @param enrollment request contains information on the neighbor and on the 
	 * enrollment request event
	 * @param portId
	 */
	public synchronized void initiateEnrollment(EnrollmentRequest enrollmentRequest, int portId) throws IPCException{
		this.enrollmentRequest = enrollmentRequest;
		remotePeer = enrollmentRequest.getNeighbor();
		switch(state){
		case NULL:
			try{
				ApplicationProcessNamingInformation apNamingInfo = ipcProcess.getName();
				CDAPMessage requestMessage = cdapSessionManager.getOpenConnectionRequestMessage(
						portId, AuthTypes.AUTH_NONE, null, null, IPCProcess.MANAGEMENT_AE, 
						remotePeer.getName().getProcessInstance(), remotePeer.getName().getProcessName(), null, 
						IPCProcess.MANAGEMENT_AE, apNamingInfo.getProcessInstance(), apNamingInfo.getProcessName());
				
				ribDaemon.sendMessage(requestMessage, portId, null);
				this.portId = portId;

				//Set timer
				timerTask = getEnrollmentFailedTimerTask(CONNECT_RESPONSE_TIMEOUT, true);
				timer.schedule(timerTask, timeout);

				//Update state
				this.state = State.WAIT_CONNECT_RESPONSE;
			}catch(Exception ex){
				ex.printStackTrace();
				this.abortEnrollment(remotePeer.getName(), portId, ex.getMessage(), true, false);
			}
			break;
		default:
			throw new IPCException("Enrollment state machine not in NULL state");
		}
	}
	
	/**
	 * Called by the EnrollmentTask when it got an M_CONNECT_R message
	 * @param cdapMessage
	 * @param cdapSessionDescriptor
	 */
	public synchronized void connectResponse(CDAPMessage cdapMessage, CDAPSessionDescriptor cdapSessionDescriptor){
		switch(state){
		case WAIT_CONNECT_RESPONSE:
			handleConnectResponse(cdapMessage);
			break;
		default:
			this.abortEnrollment(remotePeer.getName(), portId, "Message received in wrong order", true, true);
			break;
		}
	}
	
	private void handleConnectResponse(CDAPMessage cdapMessage){
		timerTask.cancel();

		if (cdapMessage.getResult() != 0){
			this.state = State.NULL;
			enrollmentTask.enrollmentFailed(remotePeer.getName(), portId, cdapMessage.getResultReason(), true, true);
			return;
		}
		
		remotePeer.getName().setProcessInstance(cdapMessage.getSrcApInst());

		//Send M_START with EnrollmentInformation object
		try{
			EnrollmentInformationRequest eiRequest = new EnrollmentInformationRequest();
			
			ApplicationRegistrationVector applicationRegistrations = rina.getExtendedIPCManager().getRegisteredApplications();
			Iterator<ApplicationProcessNamingInformation> iterator = null;
			List<ApplicationProcessNamingInformation> difsList = new ArrayList<ApplicationProcessNamingInformation>();
			for(int i=0; i<applicationRegistrations.size(); i++) {
				iterator = applicationRegistrations.get(i).getDIFNames().iterator();
				while (iterator.hasNext()) {
					difsList.add(iterator.next());
				}
			}
			eiRequest.setSupportingDifs(difsList);
			
			Long address = ipcProcess.getAddress();
			if (address != null){
				wasDIFMemberBeforeEnrollment = true;
				eiRequest.setAddress(ipcProcess.getAddress());
			} else {
				difInformation = new DIFInformation();
				if (enrollmentRequest.getEvent() != null) {
					difInformation.setDifName(
							enrollmentRequest.getEvent().getDifName());
				}
				
				ipcProcess.setDIFInformation(difInformation);
			}
			ObjectValue objectValue = new ObjectValue();
			objectValue.setByteval(encoder.encode(eiRequest));
			CDAPMessage startMessage = cdapSessionManager.getStartObjectRequestMessage(portId, null, null, 
					EnrollmentInformationRequest.ENROLLMENT_INFO_OBJECT_CLASS, objectValue, 0, 
					EnrollmentInformationRequest.ENROLLMENT_INFO_OBJECT_NAME, 0, true);
			sendCDAPMessage(startMessage);

			//Set timer
			timerTask = getEnrollmentFailedTimerTask(START_RESPONSE_TIMEOUT, true);
			timer.schedule(timerTask, timeout);

			//Update state
			state = State.WAIT_START_ENROLLMENT_RESPONSE;
		}catch(Exception ex){
			ex.printStackTrace();
			//TODO what to do?
		}
	}
	
	/**
	 * Received the Start Response
	 */
	@Override
	public void startResponse(CDAPMessage cdapMessage, CDAPSessionDescriptor cdapSessionDescriptor)
			throws RIBDaemonException {
		if (!isValidPortId(cdapSessionDescriptor)){
			return;
		}
		
		synchronized(this){
			switch(state){
			case WAIT_START_ENROLLMENT_RESPONSE:
				handleStartEnrollmentResponse(cdapMessage);
				break;
			default:
				this.abortEnrollment(remotePeer.getName(), portId, START_RESPONSE_IN_BAD_STATE, true, true);
				break;
			}
		}
	}
	
	/**
	 * Received Start enrollment response.
	 * @param cdapMessage
	 */
	private void handleStartEnrollmentResponse(CDAPMessage cdapMessage){
		//Cancel timer
		timerTask.cancel();
		
		if (cdapMessage.getResult() != 0){
			this.setState(State.NULL);
			enrollmentTask.enrollmentFailed(remotePeer.getName(), portId, cdapMessage.getResultReason(), true, true);
			return;
		}
		
		//Update address
		if (cdapMessage.getObjValue() != null){
			try{
				long address = ((EnrollmentInformationRequest) encoder.decode(
						cdapMessage.getObjValue().getByteval(), EnrollmentInformationRequest.class)).getAddress();
				ribDaemon.write(AddressRIBObject.ADDRESS_RIB_OBJECT_CLASS, AddressRIBObject.ADDRESS_RIB_OBJECT_NAME, 
						new Long(address), null);
			}catch(Exception ex){
				ex.printStackTrace();
				this.abortEnrollment(remotePeer.getName(), portId, UNEXPECTED_ERROR + ex.getMessage(), true, true);
			}
		}
		
		//Set timer
		timerTask = getEnrollmentFailedTimerTask(STOP_ENROLLMENT_TIMEOUT, true);
		timer.schedule(timerTask, timeout);
		
		//Update state
		state = State.WAIT_STOP_ENROLLMENT_RESPONSE;
	}
	
	/**
	 * A stop request was received
	 * @param cdapMessage
	 * @param cdapSessionDescriptor
	 */
	public void stop(CDAPMessage cdapMessage, CDAPSessionDescriptor cdapSessionDescriptor){
		if (!isValidPortId(cdapSessionDescriptor)){
			return;
		}
		
		synchronized(this){
			switch(state){
			case WAIT_STOP_ENROLLMENT_RESPONSE:
				handleStopEnrollment(cdapMessage);
				break;
			default:
				this.abortEnrollment(remotePeer.getName(), portId, STOP_IN_BAD_STATE, true, true);
				break;
			}
		}
	}
	
	/**
	 * Stop enrollment request received. Check if I have enough information, if not 
	 * ask for more with M_READs.
	 * Have to check if I can start operating (if not wait 
	 * until M_START operationStatus). If I can start and have enough information, 
	 * create or update all the objects received during the enrollment phase.
	 * @param cdapMessage
	 */
	private void handleStopEnrollment(CDAPMessage cdapMessage){
		//Cancel timer
		timerTask.cancel();
		
		//Check if I'm allowed to start early
		if (cdapMessage.getObjValue() == null){
			this.abortEnrollment(remotePeer.getName(), portId, STOP_WITH_NO_OBJECT_VALUE, true, true);
			return;
		}
		allowedToStart = cdapMessage.getObjValue().isBooleanval();
		stopEnrollmentRequestMessage = cdapMessage;
		
		//Request more information or start
		try{
			requestMoreInformationOrStart();
		}catch(Exception ex){
			log.error(ex.getMessage());
			this.abortEnrollment(remotePeer.getName(), portId, UNEXPECTED_ERROR + ex.getMessage(), true, true);
		}
	}
	
	/**
	 * See if more information is required for enrollment, or if we can 
	 * start or if we have to wait for the start message
	 * @throws Exception
	 */
	private void requestMoreInformationOrStart() throws Exception{
		CDAPMessage stopResponseMessage = null;
		
		//Check if more information is required
		CDAPMessage readMessage = nextObjectRequired();
		
		if (readMessage != null){
			//Request information
			sendCDAPMessage(readMessage);
			
			//Set timer
			timerTask = getEnrollmentFailedTimerTask(READ_RESPONSE_TIMEOUT, true);
			timer.schedule(timerTask, timeout);
			
			//Update state
			state = State.WAIT_READ_RESPONSE;
			return;
		}
		
		//No more information is required, if I'm allowed to start early, 
		//commit the enrollment information, set operational status to true
		//and send M_STOP_R. If not, just send M_STOP_R
		if (allowedToStart){
			try{
				commitEnrollment();
				stopResponseMessage = cdapSessionManager.getStopObjectResponseMessage(portId, null, 0, 
						null, stopEnrollmentRequestMessage.getInvokeID());
				sendCDAPMessage(stopResponseMessage);
				
				enrollmentCompleted();
			}catch(RIBDaemonException ex){
				log.error(ex);
				stopResponseMessage = cdapSessionManager.getStopObjectResponseMessage(portId, null, 3, 
						PROBLEMS_COMITING_ENROLLMENT_INFO, stopEnrollmentRequestMessage.getInvokeID());
				sendCDAPMessage(stopResponseMessage);
				this.abortEnrollment(remotePeer.getName(), portId, PROBLEMS_COMITING_ENROLLMENT_INFO, true, true);
			}
			
			return;
		}
		
		stopResponseMessage = cdapSessionManager.getStopObjectResponseMessage(portId, null, 0, 
					null, stopEnrollmentRequestMessage.getInvokeID());
		sendCDAPMessage(stopResponseMessage);
		timerTask = getEnrollmentFailedTimerTask(START_TIMEOUT, true);
		timer.schedule(timerTask, timeout);
		this.setState(State.WAIT_START);
	}
	
	/**
	 * Checks if more information is required for enrollment
	 * (At least there must be DataTransferConstants, a QoS cube and a DAF Member). If there is, 
	 * it returns a CDAP READ message requesting the next object to be read. If not, it returns null
	 * @return A CDAP READ message requesting the next object to be read. If not, it returns null
	 */
	private CDAPMessage nextObjectRequired() throws Exception{
		CDAPMessage cdapMessage = null;
		
		if (!difInformation.getDifConfiguration().getEfcpConfiguration().getDataTransferConstants().isInitialized()) {
			cdapMessage = cdapSessionManager.getReadObjectRequestMessage(portId, null, null, 
					DataTransferConstantsRIBObject.DATA_TRANSFER_CONSTANTS_RIB_OBJECT_CLASS, 0, 
					DataTransferConstantsRIBObject.DATA_TRANSFER_CONSTANTS_RIB_OBJECT_NAME, 0, true);
		} else if (difInformation.getDifConfiguration().getEfcpConfiguration().getQosCubes().size() == 0){
			cdapMessage = cdapSessionManager.getReadObjectRequestMessage(portId, null, null, 
					QoSCubeSetRIBObject.QOSCUBE_SET_RIB_OBJECT_CLASS, 0, 
					QoSCubeSetRIBObject.QOSCUBE_SET_RIB_OBJECT_NAME, 0, true);
		}else if (ipcProcess.getNeighbors().size() == 0){
			cdapMessage = cdapSessionManager.getReadObjectRequestMessage(portId, null, null, 
					NeighborSetRIBObject.NEIGHBOR_SET_RIB_OBJECT_CLASS, 0, 
					NeighborSetRIBObject.NEIGHBOR_SET_RIB_OBJECT_NAME, 0, true);
		}
		
		return cdapMessage;
	}
	
	/**
	 * Create the objects in the RIB
	 * @throws RIBDaemonException
	 */
	private void commitEnrollment() throws RIBDaemonException{
		ribDaemon.start(RIBObjectNames.OPERATIONAL_STATUS_RIB_OBJECT_CLASS, 
				RIBObjectNames.OPERATIONAL_STATUS_RIB_OBJECT_NAME, null);
	}
	
	/**
	 * Received a Read Response message
	 */
	@Override
	public void readResponse(CDAPMessage cdapMessage, CDAPSessionDescriptor cdapSessionDescriptor)
			throws RIBDaemonException {
		if (!isValidPortId(cdapSessionDescriptor)){
			return;
		}
		
		synchronized(this){
			switch(state){
			case WAIT_READ_RESPONSE:
				handleReadResponse(cdapMessage);
				break;
			default:
				this.abortEnrollment(remotePeer.getName(), portId, READ_RESPONSE_IN_BAD_STATE, true, true);
				break;
			}
		}
	}
	
	/**
	 * See if the response is valid and contains an object. See if more objects 
	 * are required. If not, 
	 * @param cdapMessage
	 */
	private void handleReadResponse(CDAPMessage cdapMessage){
		//Cancel timer
		timerTask.cancel();
		
		//Check if I'm allowed to start early
		if (cdapMessage.getResult() != 0 || cdapMessage.getObjValue() == null){
			this.abortEnrollment(remotePeer.getName(), portId, UNSUCCESSFULL_READ_RESPONSE, true, true);
			return;
		}
		
		//Update the enrollment information with the new value
		if (cdapMessage.getResult() != 0 || cdapMessage.getObjValue() == null){
			this.abortEnrollment(remotePeer.getName(), portId, UNSUCCESSFULL_READ_RESPONSE, true, true);
			return;
		}
		
		if (cdapMessage.getObjName().equals(
				DataTransferConstantsRIBObject.DATA_TRANSFER_CONSTANTS_RIB_OBJECT_NAME)){
			try{
				DataTransferConstants constants = (DataTransferConstants) encoder.decode(
						cdapMessage.getObjValue().getByteval(), DataTransferConstants.class);
				ribDaemon.create(cdapMessage.getObjClass(), cdapMessage.getObjInst(), 
						cdapMessage.getObjName(), constants, null);
			}catch(Exception ex){
				log.error(ex);
			}
		}else if (cdapMessage.getObjName().equals(QoSCubeSetRIBObject.QOSCUBE_SET_RIB_OBJECT_NAME)){
			try{
				QoSCube[] cubesArray = (QoSCube[]) encoder.decode(
						cdapMessage.getObjValue().getByteval(), QoSCube[].class);
				ribDaemon.create(cdapMessage.getObjClass(), cdapMessage.getObjInst(), 
						cdapMessage.getObjName(), cubesArray, null);
			}catch(Exception ex){
				log.error(ex);
			}
		}else if (cdapMessage.getObjName().equals(
				NeighborSetRIBObject.NEIGHBOR_SET_RIB_OBJECT_NAME)){
			try{
				Neighbor[] neighborsArray = (Neighbor[]) encoder.decode(
						cdapMessage.getObjValue().getByteval(), Neighbor[].class);
				ribDaemon.create(cdapMessage.getObjClass(), cdapMessage.getObjInst(), 
						cdapMessage.getObjName(), neighborsArray, null);
			}catch(Exception ex){
				log.error(ex);
			}
		}else{
			log.warn("The object to be created is not required for enrollment: "+cdapMessage.toString());
		}
		
		//Request more information or proceed with the enrollment program
		try{
			requestMoreInformationOrStart();
		}catch(Exception ex){
			log.error(ex);
			this.abortEnrollment(remotePeer.getName(), portId, UNEXPECTED_ERROR + ex.getMessage(), true, true);
		}
	}
	
	/**
	 * A stop request was received
	 * @param cdapMessage
	 * @param cdapSessionDescriptor
	 */
	public void start(CDAPMessage cdapMessage, CDAPSessionDescriptor cdapSessionDescriptor){
		if (!isValidPortId(cdapSessionDescriptor)){
			return;
		}

		synchronized(this){
			switch(state){
			case WAIT_START:
				handleStartOperation(cdapMessage);
				break;
			case ENROLLED:
				//Do nothing, just ignore
				break;
			default:
				this.abortEnrollment(remotePeer.getName(), portId, START_IN_BAD_STATE, true, true);
				break;
			}
		}
	}
	
	/**
	 * Commit the enrollment information and
	 * start.
	 */
	private void handleStartOperation(CDAPMessage cdapMessage){
		//Cancel timer
		timerTask.cancel();
		
		if (cdapMessage.getResult() != 0){
			this.abortEnrollment(remotePeer.getName(), portId, UNSUCCESSFULL_START, true, true);
			return;
		}
		
		try{
			commitEnrollment();
			enrollmentCompleted();
		}catch(Exception ex){
			log.error(ex);
			this.abortEnrollment(remotePeer.getName(), portId, PROBLEMS_COMITING_ENROLLMENT_INFO, true, true);
		}
	}
	
	private void enrollmentCompleted(){
		synchronized(this){
			timer.cancel();
			this.setState(State.ENROLLED);
		}
		
		//Create or update the neighbor information in the RIB
		createOrUpdateNeighborInformation(true);
		
		//Send DirectoryForwardingTableEntries
		try {
			sendCreateInformation(DirectoryForwardingTableEntrySetRIBObject.DIRECTORY_FORWARDING_TABLE_ENTRY_SET_RIB_OBJECT_CLASS, 
					DirectoryForwardingTableEntrySetRIBObject.DIRECTORY_FORWARDING_ENTRY_SET_RIB_OBJECT_NAME);
		} catch (Exception e) {
			log.error("Problems sending DirectoryForwardingTableEntries to new neighbor: "+e.getMessage());
		}
		
		enrollmentTask.enrollmentCompleted(remotePeer, true);
		
		//Notify the kernel
		if (!wasDIFMemberBeforeEnrollment) {
			try {
				rina.getKernelIPCProcess().assignToDIF(ipcProcess.getDIFInformation());
			} catch(Exception ex) {
				log.error("Problems communicating with the Kernel components of the IPC Processs: " 
						+ ex.getMessage());
			}
		}

		//Notify the IPC Manager
		if (enrollmentRequest != null && enrollmentRequest.getEvent() != null){
			try {
				NeighborList list = new NeighborList();
				list.addFirst(enrollmentRequest.getNeighbor());
				rina.getExtendedIPCManager().enrollToDIFResponse(
						enrollmentRequest.getEvent(), 0, list, 
						ipcProcess.getDIFInformation());
			} catch (Exception ex) {
				log.error("Problems sending message to IPC Manager: "+ex.getMessage());
			}
		}

		log.info("Remote IPC Process enrolled!");
	}
}
