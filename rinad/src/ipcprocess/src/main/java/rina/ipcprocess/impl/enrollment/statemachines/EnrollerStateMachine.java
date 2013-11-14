package rina.ipcprocess.impl.enrollment.statemachines;

import java.util.List;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import eu.irati.librina.ApplicationProcessNamingInformation;
import eu.irati.librina.DataTransferConstants;
import eu.irati.librina.Neighbor;
import eu.irati.librina.QoSCube;

import rina.applicationprocess.api.WhatevercastName;
import rina.cdap.api.CDAPException;
import rina.cdap.api.CDAPSessionDescriptor;
import rina.cdap.api.CDAPSessionManager;
import rina.cdap.api.message.CDAPMessage;
import rina.cdap.api.message.ObjectValue;
import rina.encoding.api.Encoder;
import rina.enrollment.api.EnrollmentInformationRequest;
import rina.enrollment.api.EnrollmentTask;
import rina.flowallocator.api.DirectoryForwardingTable;
import rina.ipcprocess.impl.IPCProcess;
import rina.ipcprocess.impl.enrollment.ribobjects.NeighborSetRIBObject;
import rina.ribdaemon.api.RIBDaemon;
import rina.ribdaemon.api.RIBDaemonException;
import rina.ribdaemon.api.RIBObject;
import rina.ribdaemon.api.RIBObjectNames;

/**
 * The state machine of the party that is a member of the DIF
 * and will help the joining party (enrollee) to join the DIF.
 * @author eduardgrasa
 *
 */
public class EnrollerStateMachine extends BaseEnrollmentStateMachine{
	
	private static final Log log = LogFactory.getLog(EnrollerStateMachine.class);
	
	private IPCProcess ipcProcess = null;
	
	public EnrollerStateMachine(RIBDaemon ribDaemon, CDAPSessionManager cdapSessionManager, Encoder encoder, 
			ApplicationProcessNamingInformation remoteNamingInfo, EnrollmentTask enrollmentTask, long timeout){
		super(ribDaemon, cdapSessionManager, encoder, remoteNamingInfo, enrollmentTask, timeout);
		ipcProcess = IPCProcess.getInstance();
	}
	
	/**
	 * An M_CONNECT message has been received
	 * @param cdapMessage
	 * @param portId
	 */
	public void connect(CDAPMessage cdapMessage, int portId) {
		synchronized(this){
			switch(this.state){
			case NULL:
				handleNullState(cdapMessage, portId);
				break;
			default:
				this.abortEnrollment(remotePeer.getName(), portId, CONNECT_IN_NOT_NULL, false, true);
				break;
			}
		}
	}
	
	/**
	 * Handle the transition from the NULL to the WAIT_START_ENROLLMENT state.
	 * Authenticate the remote peer and issue a connect response
	 * @param cdapMessage
	 * @param portId
	 */
	private void handleNullState(CDAPMessage cdapMessage, int portId){
		CDAPMessage outgoingCDAPMessage = null;
		this.portId = portId;
		log.debug(portId);

		log.debug("Authenticating PC process "+cdapMessage.getSrcApName()+" "+cdapMessage.getSrcApInst());
		remotePeer.getName().setProcessName(cdapMessage.getSrcApName());
		remotePeer.getName().setProcessInstance(cdapMessage.getSrcApInst());

		//TODO authenticate sender
		log.debug("Authentication successfull");

		//Send M_CONNECT_R
		try{
			outgoingCDAPMessage = cdapSessionManager.getOpenConnectionResponseMessage(portId, cdapMessage.getAuthMech(), 
					cdapMessage.getAuthValue(), cdapMessage.getSrcAEInst(), 
					IPCProcess.MANAGEMENT_AE, cdapMessage.getSrcApInst(), cdapMessage.getSrcApName(), 0, null, cdapMessage.getDestAEInst(), 
					IPCProcess.MANAGEMENT_AE, ipcProcess.getName().getProcessInstance(), 
					ipcProcess.getName().getProcessName(), cdapMessage.getInvokeID());

			sendCDAPMessage(outgoingCDAPMessage);

			//set timer (max time to wait before getting M_START)
			timerTask = getEnrollmentFailedTimerTask(START_ENROLLMENT_TIMEOUT, false);
			timer.schedule(timerTask, timeout);
			log.debug("Waiting for start enrollment request message");

			this.setState(State.WAIT_START_ENROLLMENT);
		}catch(CDAPException ex){
			log.error(ex);
		}
	}
	
	/**
	 * Called by the Enrollment object when it receives an M_START message from 
	 * the enrolling member
	 * @param cdapMessage
	 * @param cdapSessionDescriptor
	 */
	public void start(CDAPMessage cdapMessage, CDAPSessionDescriptor cdapSessionDescriptor) {
		if (!isValidPortId(cdapSessionDescriptor)){
			return;
		}
		
		synchronized(this){
			switch(state){
			case WAIT_START_ENROLLMENT:
				handleStartEnrollment(cdapMessage);
				break;
			default:
				this.abortEnrollment(remotePeer.getName(), portId, START_IN_BAD_STATE, false, true);
				break;
			}
		}
	}
	
	/**
	 * Have to look at the enrollment information request, from that deduce if the IPC process 
	 * requesting to enroll with me is already a member of the DIF and if its address is valid.
	 * If it is not a member of the DIF, send a new address with the M_START_R, send the 
	 * M_CREATEs to provide the DIF initialization information and state, and send M_STOP_R.
	 * IF it is a valid member, just send M_START_R with no address and M_STOP_R
	 * @param cdapMessage
	 */
	private void handleStartEnrollment(CDAPMessage cdapMessage){
		EnrollmentInformationRequest eiRequest = null;
		boolean requiresInitialization = false;
		CDAPMessage responseMessage = null;
		ObjectValue objectValue = null;

		//Cancel timer
		timerTask.cancel();

		//Find out if the enrolling IPC process requires initialization
		try{
			if (cdapMessage.getObjValue() == null){
				requiresInitialization = true;
			}else{
				eiRequest = (EnrollmentInformationRequest) encoder.
				decode(cdapMessage.getObjValue().getByteval(), EnrollmentInformationRequest.class);
				if (!isValidAddress(eiRequest.getAddress())){
					requiresInitialization = true;
				}
			}
		}catch(Exception ex){
			ex.printStackTrace();
			this.sendNegativeStartResponseAndAbortEnrollment(-1, ex.getMessage(), cdapMessage);
			return;
		}

		
		try{
			//Send M_START_R
			if (requiresInitialization){
				long address = getValidAddress();
				if (address == -1){
					this.sendNegativeStartResponseAndAbortEnrollment(-1, "Could not assign a valid address", cdapMessage);
					return;
				}
				
				objectValue = new ObjectValue();
				eiRequest.setAddress(address);
				objectValue.setByteval(encoder.encode(eiRequest));
			}

			responseMessage =
				cdapSessionManager.getStartObjectResponseMessage(this.portId, null, cdapMessage.getObjClass(), objectValue, 0, 
						cdapMessage.getObjName(), 0, null, cdapMessage.getInvokeID());
			sendCDAPMessage(responseMessage);
			this.remotePeer.setAddress(eiRequest.getAddress());

			//If initialization is required send the M_CREATEs
			if (requiresInitialization){
				sendDIFStaticInformation();
			}

			sendDIFDynamicInformation();
			
			//Send the M_STOP request
			objectValue = new ObjectValue();
			objectValue.setBooleanval(true);
			responseMessage = cdapSessionManager.getStopObjectRequestMessage(this.portId, null, null, EnrollmentInformationRequest.ENROLLMENT_INFO_OBJECT_CLASS, 
					objectValue, 0, EnrollmentInformationRequest.ENROLLMENT_INFO_OBJECT_NAME, 0, true);
			sendCDAPMessage(responseMessage);
			timerTask = getEnrollmentFailedTimerTask(STOP_ENROLLMENT_RESPONSE_TIMEOUT, false);
			timer.schedule(timerTask, timeout);
			log.debug("Waiting for stop enrollment response message");

			this.setState(State.WAIT_STOP_ENROLLMENT_RESPONSE);
		}catch(Exception ex){
			log.error(ex);
			this.abortEnrollment(remotePeer.getName(), portId, UNEXPECTED_ERROR + ex.getMessage(), false, true);
		}
	}
	
	/**
	 * Send a negative response to the M_START enrollment message
	 * @param result the error code
	 * @param resultReason the reason of the bad result
	 * @param requestMessage the received M_START enrollment message
	 */
	private void sendNegativeStartResponseAndAbortEnrollment(int result, String resultReason, CDAPMessage requestMessage){
		try{
			CDAPMessage responseMessage =
				cdapSessionManager.getStartObjectResponseMessage(this.portId, null, requestMessage.getObjClass(), null, 0, 
						requestMessage.getObjName(), result, resultReason, requestMessage.getInvokeID());
			sendCDAPMessage(responseMessage);
			this.abortEnrollment(remotePeer.getName(), portId, resultReason, false, true);
		}catch(Exception ex){
			ex.printStackTrace();
			log.error(ex);
		}
	}
	
	/**
	 * Decides if a given address is valid or not
	 * @param address
	 * @return
	 */
	private boolean isValidAddress(long address) {
		//Check if the address is negative
		if (address <= 0){
			return false;
		}
		
		//Check if we know the remote IPC Process
		RINAConfiguration rinaConf = RINAConfiguration.getInstance();
		KnownIPCProcessAddress ipcAddress = rinaConf.getIPCProcessAddress(this.enrollmentTask.getIPCProcess().getDIFName(),
				this.remotePeer.getApplicationProcessName(), 
				this.remotePeer.getApplicationProcessInstance());
		if (ipcAddress != null){
			if (ipcAddress.getAddress() == address){
				return true;
			}else{
				return false;
			}
		}
		
		//Check if we know the prefix assigned to the organization
		long prefix = rinaConf.getAddressPrefixConfiguration(this.enrollmentTask.getIPCProcess().getDIFName(), 
				this.remotePeer.getApplicationProcessName());
		if (prefix == -1){
			//We don't know the organization of the IPC Process
			return false;
		}
		
		//Check if the address is within the range of the prefix
		if (address < prefix || address >= prefix + AddressPrefixConfiguration.MAX_ADDRESSES_PER_PREFIX){
			return false;
		}
		
		//Check if the address is in use
		List<Neighbor> neighbors = this.ribDaemon.getIPCProcess().getNeighbors();
		for(int i=0; i<neighbors.size(); i++){
			if(neighbors.get(i).getAddress() == address){
				if (neighbors.get(i).getApplicationProcessName().equals(this.remotePeer.getApplicationProcessName())){
					//We knew about this IPC Process
					return true;
				}else{
					//The address is in use by another IPC Process
					return false;
				}
			}
		}
		
		return true;
	}
	
	/**
	 * Return a valid address for the IPC process that 
	 * wants to join the DIF
	 * @return
	 */
	private long getValidAddress(){
		RINAConfiguration rinaConf = RINAConfiguration.getInstance();
		
		//See if we know the configuration of the remote IPC Process
		KnownIPCProcessAddress ipcAddress = rinaConf.getIPCProcessAddress(this.enrollmentTask.getIPCProcess().getDIFName(),
				this.remotePeer.getApplicationProcessName(), 
				this.remotePeer.getApplicationProcessInstance());
		if (ipcAddress != null){
			return ipcAddress.getAddress();
		}
		
		//See if we know the prefix of the remote IPC Process
		long prefix = rinaConf.getAddressPrefixConfiguration(this.enrollmentTask.getIPCProcess().getDIFName(), 
				this.remotePeer.getApplicationProcessName());
		if (prefix == -1){
			//We don't know the prefix, return an invalid address indicating 
			//that the IPC Process cannot join the DIF
			return prefix;
		}else{
			//Get an address that is not in use
			List<Neighbor> neighbors = this.ribDaemon.getIPCProcess().getNeighbors();
			long candidateAddress = prefix;
			while(candidateAddress < prefix + AddressPrefixConfiguration.MAX_ADDRESSES_PER_PREFIX){
				for(int i=0; i<neighbors.size(); i++){
					if (neighbors.get(i).getAddress() == candidateAddress){
						//The candidate address is in use
						candidateAddress++;
						break;
					}
				}
				
				//The candidate address is not in use
				return candidateAddress;
			}
		}
		
		//We could not find a valid address, return an invalid one indicating that
		//the IPC Process cannot join the DIF
		return -1;
	}
	
	/**
	 * Send all the information required to start operation to 
	 * the IPC process that is enrolling to me
	 * @throws Exception
	 */
	private void sendDIFStaticInformation() throws Exception{
		//Send whatevercast names
		sendCreateInformation(WhatevercastName.WHATEVERCAST_NAME_SET_RIB_OBJECT_CLASS, 
				WhatevercastName.WHATEVERCAST_NAME_SET_RIB_OBJECT_NAME);
		
		//Send data transfer constants
		sendCreateInformation(DataTransferConstants.DATA_TRANSFER_CONSTANTS_RIB_OBJECT_CLASS, 
				DataTransferConstants.DATA_TRANSFER_CONSTANTS_RIB_OBJECT_NAME);
		
		//Send QoS Cubes
		sendCreateInformation(QoSCube.QOSCUBE_SET_RIB_OBJECT_CLASS, 
				QoSCube.QOSCUBE_SET_RIB_OBJECT_NAME);
	}
	
	/**
	 * Send all the DIF dynamic information
	 * @throws Exception
	 */
	private void sendDIFDynamicInformation() throws Exception{
		//Send DirectoryForwardingTableEntries
		sendCreateInformation(DirectoryForwardingTable.DIRECTORY_FORWARDING_TABLE_ENTRY_SET_RIB_OBJECT_CLASS, 
				DirectoryForwardingTable.DIRECTORY_FORWARDING_ENTRY_SET_RIB_OBJECT_NAME);
		
		//Send neighbors (including myself)
		RIBObject neighborSet = ribDaemon.read(
				NeighborSetRIBObject.NEIGHBOR_SET_RIB_OBJECT_CLASS, 
				NeighborSetRIBObject.NEIGHBOR_SET_RIB_OBJECT_NAME);
		List<RIBObject> neighbors = neighborSet.getChildren();
		
		Neighbor[] neighborsArray = new Neighbor[neighbors.size() + 1];
		for(int i=1; i<=neighbors.size(); i++){
			neighborsArray[i] = (Neighbor) neighbors.get(i-1).getObjectValue();
		}
		
		neighborsArray[0] = new Neighbor();
		neighborsArray[0].setAddress(ipcProcess.getAddress().longValue());
		neighborsArray[0].setName(ipcProcess.getName());
		
		ObjectValue objectValue = new ObjectValue();
		objectValue.setByteval(encoder.encode(neighborsArray));
		CDAPMessage cdapMessage = cdapSessionManager.getCreateObjectRequestMessage(
				this.portId, null, null, neighborSet.getObjectClass(), 
				0, NeighborSetRIBObject.NEIGHBOR_SET_RIB_OBJECT_NAME, objectValue, 0, false);
		sendCDAPMessage(cdapMessage);
	}
	
	/**
	 * Gets the object value from the RIB and send it as a CDAP Mesage
	 * @param objectClass the class of the object to be send
	 * @param objectName the name of the object to be send
	 * @param suffix the suffix to send after enrollment info
	 * @throws Exception
	 */
	private void sendCreateInformation(String objectClass, String objectName) throws Exception{
		RIBObject ribObject = null;
		CDAPMessage cdapMessage = null;
		ObjectValue objectValue = null;
		
		ribObject = ribDaemon.read(objectClass, objectName);
		objectValue = new ObjectValue();
		objectValue.setByteval(encoder.encode(ribObject.getObjectValue()));
		cdapMessage = cdapSessionManager.getCreateObjectRequestMessage(this.portId, null, null, objectClass, 
				ribObject.getObjectInstance(), objectName, objectValue, 0, false);
		sendCDAPMessage(cdapMessage);
	}

	/**
	 * The response of the stop operation has been received
	 * @param cdapMessage
	 * @param cdapSessionDescriptor
	 */
	@Override
	public void stopResponse(CDAPMessage cdapMessage, CDAPSessionDescriptor cdapSessionDescriptor)
			throws RIBDaemonException {
		if (!isValidPortId(cdapSessionDescriptor)){
			return;
		}
		
		synchronized(this){
			switch(state){
			case WAIT_STOP_ENROLLMENT_RESPONSE:
				handleStopEnrollmentResponse(cdapMessage);
				break;
			default:
				this.abortEnrollment(remotePeer.getName(), portId, STOP_RESPONSE_IN_BAD_STATE, false, true);
				break;
			}
		}
	}
	
	/**
	 * The response of the stop operation has been received, send M_START operation without 
	 * waiting for an answer and consider the process enrolled
	 * @param cdapMessage
	 * @param cdapSessionDescriptor
	 */
	private void handleStopEnrollmentResponse(CDAPMessage cdapMessage){
		//Cancel timer
		timerTask.cancel();

		if (cdapMessage.getResult() != 0){
			this.setState(State.NULL);
			enrollmentTask.enrollmentFailed(remotePeer.getName(), portId, cdapMessage.getResultReason(), false, true);
		}else{
			try{
				CDAPMessage startMessage = cdapSessionManager.getStartObjectRequestMessage(portId, null, null, 
						RIBObjectNames.OPERATIONAL_STATUS_RIB_OBJECT_CLASS, null, 0, 
						RIBObjectNames.OPERATIONAL_STATUS_RIB_OBJECT_NAME, 0, false);
				sendCDAPMessage(startMessage);
			}catch(Exception ex){
				log.error(ex);
			}
			
			enrollmentCompleted(false);
		}
	}
}
