package rina.ipcprocess.impl.ribdaemon;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import eu.irati.librina.KernelIPCProcessSingleton;
import eu.irati.librina.QueryRIBRequestEvent;
import eu.irati.librina.RIBObjectList;
import eu.irati.librina.rina;

import rina.cdap.api.CDAPException;
import rina.cdap.api.CDAPMessageHandler;
import rina.cdap.api.CDAPSession;
import rina.cdap.api.CDAPSessionDescriptor;
import rina.cdap.api.CDAPSessionManager;
import rina.cdap.api.message.CDAPMessage;
import rina.cdap.api.message.CDAPMessage.Flags;
import rina.cdap.api.message.CDAPMessage.Opcode;
import rina.cdap.api.message.ObjectValue;
import rina.encoding.api.Encoder;
import rina.enrollment.api.EnrollmentTask;
import rina.events.api.Event;
import rina.events.api.EventListener;
import rina.ipcprocess.api.IPCProcess;
import rina.ipcprocess.impl.events.NMinusOneFlowAllocatedEvent;
import rina.ipcprocess.impl.events.NMinusOneFlowDeallocatedEvent;
import rina.ipcprocess.impl.ribdaemon.rib.RIB;
import rina.resourceallocator.api.NMinus1FlowManager;
import rina.ribdaemon.api.NotificationPolicy;
import rina.ribdaemon.api.RIBDaemonException;
import rina.ribdaemon.api.RIBObject;
import rina.ribdaemon.api.RIBObjectNames;
import rina.ribdaemon.api.UpdateStrategy;

/**
 * RIBDaemon that stores the objects in memory
 * @author eduardgrasa
 *
 */
public class RIBDaemonImpl extends BaseRIBDaemon implements EventListener{
	
	private static final Log log = LogFactory.getLog(RIBDaemonImpl.class);
	
	/** Create, retrieve and delete CDAP sessions **/
	private CDAPSessionManager cdapSessionManager = null;
	
	/** Encode and decode objects **/
	private Encoder encoder = null;
	
	/** The RIB **/
	private RIB rib = null;
	
	/** CDAP Message handlers that have sent a CDAP message and are waiting for a reply **/
	private Map<Integer, CDAPMessageHandler> messageHandlersWaitingForReply = null;
	
	/** 
	 * Lock to control that when sending a message requiring a reply the 
	 * CDAP Session manager has been updated before receiving the response message 
	 */
	private Object atomicSendLock = null;
	
	private IPCProcess ipcProcess = null;
	private KernelIPCProcessSingleton kernelIPCProcess = null;
	private ManagementSDUReader managementSduReader = null;
	
	/**
	 * The N-1 Flow Manager
	 */
	private NMinus1FlowManager nMinus1FlowManager = null;
	
	public RIBDaemonImpl(){
		rib = new RIB();
		messageHandlersWaitingForReply = new ConcurrentHashMap<Integer, CDAPMessageHandler>();
		atomicSendLock = new Object();
	}
	
	public void setIPCProcess(IPCProcess ipcProcess){
		this.ipcProcess = ipcProcess;
		encoder = ipcProcess.getEncoder();
		cdapSessionManager = ipcProcess.getCDAPSessionManager();
		nMinus1FlowManager = ipcProcess.getResourceAllocator().getNMinus1FlowManager();
		subscribeToEvents();
		kernelIPCProcess = rina.getKernelIPCProcess();
		managementSduReader = new ManagementSDUReader(kernelIPCProcess, this, 
				IPCProcess.DEFAULT_MAX_SDU_SIZE_IN_BYTES);
		ipcProcess.execute(managementSduReader);
	}
	
	private void subscribeToEvents(){
		subscribeToEvent(Event.N_MINUS_1_FLOW_ALLOCATED, this);
		subscribeToEvent(Event.N_MINUS_1_FLOW_DEALLOCATED, this);
	}

	/**
	 * The RIB Daemon has to process the CDAP message and, 
	 * if valid, it will either pass it to interested subscribers and/or write to storage and/or modify other 
	 * tasks data structures. It may be the case that the CDAP message is not addressed to an application 
	 * entity within this IPC process, then the RMT may decide to rely the message to the right destination 
	 * (after consulting an adequate forwarding table).
	 * @param cdapMessage
	 */
	protected void cdapMessageDelivered(byte[] encodedCDAPMessage, int portId){
		CDAPMessage cdapMessage = null;
		CDAPSession cdapSession = null;
		CDAPSessionDescriptor cdapSessionDescriptor = null;

		//1 Decode the message and obtain the CDAP session descriptor
		try{
			//If another thread was sending a message, let him finish
			synchronized(atomicSendLock){
				cdapMessage = cdapSessionManager.messageReceived(encodedCDAPMessage, portId);
				cdapSession = cdapSessionManager.getCDAPSession(portId);
				if (cdapSession == null) {
					log.error("Could not find CDAP open session related to portId "+portId);
					return;
				}
				
				cdapSessionDescriptor = cdapSession.getSessionDescriptor();
				log.debug("Received CDAP Message through N-1 flow identified by portId "
						+portId+":"+cdapMessage.toString());
			}
		}catch(CDAPException ex){
			log.error("Error decoding CDAP message: " + ex.getMessage());
			ex.printStackTrace();
			if (ex.getCDAPMessage() != null && ex.getCDAPMessage().getInvokeID() != 0){
				this.sendErrorMessage(ex, portId);
			}
			return;
		}
		
		Opcode opcode = cdapMessage.getOpCode();

		//2 Find the destination of the message and call it
		try{
			CDAPMessageHandler cdapMessageHandler = null;

			//M_CONNECT, M_CONNECT_R, M_RELEASE and M_RELEASE_R are handled by the Enrollment task
			if (opcode.equals(Opcode.M_CONNECT) || opcode.equals(Opcode.M_CONNECT_R) || 
					opcode.equals(Opcode.M_RELEASE) || opcode.equals(Opcode.M_RELEASE_R)){
				EnrollmentTask enrollmentTask = ipcProcess.getEnrollmentTask();
				switch(opcode){
				case M_CONNECT:
					enrollmentTask.connect(cdapMessage, cdapSessionDescriptor);
					break;
				case M_CONNECT_R:
					enrollmentTask.connectResponse(cdapMessage, cdapSessionDescriptor);
					break;
				case M_RELEASE:
					enrollmentTask.release(cdapMessage, cdapSessionDescriptor);
					break;
				case M_RELEASE_R:
					enrollmentTask.releaseResponse(cdapMessage, cdapSessionDescriptor);
					break;
				}
				
				return;
			}
			//All the other request messages (M_READ, M_WRITE, M_CREATE, M_DELETE, M_START, M_STOP, M_CANCELREAD) are 
			//handled by the RIB
			else if (opcode.equals(Opcode.M_READ) || opcode.equals(Opcode.M_WRITE) || opcode.equals(Opcode.M_CREATE) || 
					opcode.equals(Opcode.M_DELETE) || opcode.equals(Opcode.M_START) || opcode.equals(Opcode.M_STOP) || 
					opcode.equals(Opcode.M_CANCELREAD)){
				this.processOperation(cdapMessage, cdapSessionDescriptor);
			}
			//All the response messages must be handled by the entities that are waiting for the reply
			else{
				if (cdapMessage.getFlags() != null && cdapMessage.getFlags().equals(Flags.F_RD_INCOMPLETE)){
					cdapMessageHandler = messageHandlersWaitingForReply.get(cdapMessage.getInvokeID());
				}else{
					cdapMessageHandler = messageHandlersWaitingForReply.remove(cdapMessage.getInvokeID());
				}

				if (cdapMessageHandler == null){
					log.error("Nobody was waiting for this response message "+cdapMessage.toString());
				}else{
					switch(opcode){
					case M_READ_R:
						cdapMessageHandler.readResponse(cdapMessage, cdapSessionDescriptor);
						break;
					case M_WRITE_R:
						cdapMessageHandler.writeResponse(cdapMessage, cdapSessionDescriptor);
						break;
					case M_CREATE_R:
						cdapMessageHandler.createResponse(cdapMessage, cdapSessionDescriptor);
						break;
					case M_DELETE_R:
						cdapMessageHandler.deleteResponse(cdapMessage, cdapSessionDescriptor);
						break;
					case M_START_R:
						cdapMessageHandler.startResponse(cdapMessage, cdapSessionDescriptor);
						break;
					case M_STOP_R:
						cdapMessageHandler.stopResponse(cdapMessage, cdapSessionDescriptor);
						break;
					case M_CANCELREAD_R:
						cdapMessageHandler.cancelReadResponse(cdapMessage, cdapSessionDescriptor);
						break;
					}
				}
			}
		}catch(RIBDaemonException ex){
			ex.printStackTrace();
			log.error(ex);
		}
	}
	
	/**
	 * Invoked by the RIB Daemon when there has been an error decoding the contents of a 
	 * CDAP Message
	 * @param cdapException
	 * @param portId
	 */
	private void sendErrorMessage(CDAPException cdapException, int portId){
		CDAPMessage wrongMessage = cdapException.getCDAPMessage();
		CDAPMessage returnMessage = null;

		switch(wrongMessage.getOpCode()){
		case M_CONNECT:
			try{
				returnMessage = cdapSessionManager.getOpenConnectionResponseMessage(portId, wrongMessage.getAuthMech(), wrongMessage.getAuthValue(), wrongMessage.getSrcAEInst(), 
						wrongMessage.getSrcAEName(), wrongMessage.getSrcApInst(), wrongMessage.getSrcApName(), cdapException.getResult(), 
						cdapException.getResultReason(), wrongMessage.getDestAEInst(), wrongMessage.getDestAEName(), wrongMessage.getDestApInst(), 
						wrongMessage.getDestApName(), wrongMessage.getInvokeID());
			}catch(CDAPException ex){
				ex.printStackTrace();
			}
			break;
		case M_CREATE:
			try{
				returnMessage = cdapSessionManager.getCreateObjectResponseMessage(portId, wrongMessage.getFlags(), 
						wrongMessage.getObjClass(), wrongMessage.getObjInst(), wrongMessage.getObjName(), wrongMessage.getObjValue(), 
						cdapException.getResult(), cdapException.getResultReason(), wrongMessage.getInvokeID());
			}catch(CDAPException ex){
				ex.printStackTrace();
			}
			break;
		case M_DELETE:
			try{
				returnMessage = cdapSessionManager.getDeleteObjectResponseMessage(portId, wrongMessage.getFlags(), wrongMessage.getObjClass(), 
						wrongMessage.getObjInst(), wrongMessage.getObjName(), cdapException.getResult(), cdapException.getResultReason(), wrongMessage.getInvokeID());
			}catch(CDAPException ex){
				ex.printStackTrace();
			}
			break;
		case M_READ:
			try{
				returnMessage = cdapSessionManager.getReadObjectResponseMessage(portId, wrongMessage.getFlags(), wrongMessage.getObjClass(), 
						wrongMessage.getObjInst(), wrongMessage.getObjName(), wrongMessage.getObjValue(), cdapException.getResult(), 
						cdapException.getResultReason(), wrongMessage.getInvokeID());
			}catch(CDAPException ex){
				ex.printStackTrace();
			}
			break;
		case M_WRITE:
			try{
				returnMessage = cdapSessionManager.getWriteObjectResponseMessage(portId, wrongMessage.getFlags(), cdapException.getResult(), 
						cdapException.getResultReason(), wrongMessage.getInvokeID());
			}catch(CDAPException ex){
				ex.printStackTrace();
			}
			break;
		case M_CANCELREAD:
			try{
				returnMessage = cdapSessionManager.getCancelReadResponseMessage(portId, wrongMessage.getFlags(), wrongMessage.getInvokeID(), cdapException.getResult(), 
						cdapException.getResultReason());
			}catch(CDAPException ex){
				ex.printStackTrace();
			}
			break;
		case M_START:
			try{
				returnMessage = cdapSessionManager.getStartObjectResponseMessage(portId, wrongMessage.getFlags(), cdapException.getResult(), 
						cdapException.getResultReason(), wrongMessage.getInvokeID());
			}catch(CDAPException ex){
				ex.printStackTrace();
			}
			break;
		case M_STOP:
			try{
				returnMessage = cdapSessionManager.getStopObjectResponseMessage(portId, wrongMessage.getFlags(), cdapException.getResult(), 
						cdapException.getResultReason(), wrongMessage.getInvokeID());
			}catch(CDAPException ex){
				ex.printStackTrace();
			}
			break;
		case M_RELEASE:
			try{
				returnMessage = cdapSessionManager.getReleaseConnectionResponseMessage(portId, wrongMessage.getFlags(), cdapException.getResult(), 
						cdapException.getResultReason(), wrongMessage.getInvokeID());
			}catch(CDAPException ex){
				ex.printStackTrace();
			}
			break;
		}

		if (returnMessage != null){
			try{
				this.sendMessage(returnMessage, portId, null);
			}catch(Exception ex){
				ex.printStackTrace();
			}
		}
	}
	
	/**
	 * Called when an event has happened
	 */
	public void eventHappened(Event event) {
		if (event.getId().equals(Event.N_MINUS_1_FLOW_DEALLOCATED)) {
			NMinusOneFlowDeallocatedEvent flowEvent = (NMinusOneFlowDeallocatedEvent) event;
			nMinusOneFlowDeallocated(flowEvent.getPortId());
		} else if (event.getId().equals(Event.N_MINUS_1_FLOW_ALLOCATED)) {
			NMinusOneFlowAllocatedEvent flowEvent = (NMinusOneFlowAllocatedEvent) event;
			nMinusOneFlowAllocated(flowEvent);
		}
	}
	
	/**
	 * Invoked by the RMT when it detects that a certain flow has been deallocated, and therefore any CDAP sessions 
	 * over it should be terminated.
	 * @param portId identifies the flow that has been deallocated
	 */
	private void nMinusOneFlowDeallocated(int portId) {
		cdapSessionManager.removeCDAPSession(portId);
	}
	
	private void nMinusOneFlowAllocated(NMinusOneFlowAllocatedEvent event) {
		//Do nothing
	}
	
	/**
	 * Causes a CDAP message to be sent
	 * @param cdapMessage the message to be sent
	 * @param sessionId the CDAP session id
	 * @param cdapMessageHandler the class to be called when the response message is received (if required)
	 * @throws RIBDaemonException
	 */
	public void sendMessage(CDAPMessage cdapMessage, int sessionId,
			CDAPMessageHandler cdapMessageHandler) throws RIBDaemonException{
		sendMessage(false, cdapMessage, sessionId, 0, cdapMessageHandler);
	}
	
	public void sendMessageToAddress(CDAPMessage cdapMessage, int sessionId, long address,
			CDAPMessageHandler cdapMessageHandler) throws RIBDaemonException {
		sendMessage(true, cdapMessage, sessionId, address, cdapMessageHandler);
	}
	
	private void sendMessage(boolean useAddress, CDAPMessage cdapMessage, int sessionId, long address,
			CDAPMessageHandler cdapMessageHandler) throws RIBDaemonException{
		if (cdapMessage.getInvokeID() != 0 && !cdapMessage.getOpCode().equals(Opcode.M_CONNECT) 
				&& !cdapMessage.getOpCode().equals(Opcode.M_RELEASE) 
				&& !cdapMessage.getOpCode().equals(Opcode.M_CANCELREAD_R)
				&& !cdapMessage.getOpCode().equals(Opcode.M_CONNECT_R)
				&& !cdapMessage.getOpCode().equals(Opcode.M_CREATE_R) 
				&& !cdapMessage.getOpCode().equals(Opcode.M_READ_R)
				&& !cdapMessage.getOpCode().equals(Opcode.M_DELETE_R)
				&& !cdapMessage.getOpCode().equals(Opcode.M_RELEASE_R)
				&& !cdapMessage.getOpCode().equals(Opcode.M_START_R)
				&& !cdapMessage.getOpCode().equals(Opcode.M_STOP_R)
				&& !cdapMessage.getOpCode().equals(Opcode.M_WRITE_R)
				&& cdapMessageHandler == null){
			throw new RIBDaemonException(RIBDaemonException.RESPONSE_REQUIRED_BUT_MESSAGE_HANDLER_IS_NULL);
		}
		
		byte[] sdu = null;
		synchronized(atomicSendLock){
			try{
				sdu = cdapSessionManager.encodeNextMessageToBeSent(cdapMessage, sessionId);
				
				if (useAddress) {
					kernelIPCProcess.sendMgmgtSDUToAddress(sdu, sdu.length, address);
					log.debug("Sent CDAP Message to "+address+": "+cdapMessage.toString());
				} else {
					kernelIPCProcess.writeMgmgtSDUToPortId(sdu, sdu.length, sessionId);
					String destination = cdapSessionManager.getCDAPSession(sessionId).getSessionDescriptor().getDestApName();
					log.debug("Sent CDAP Message to "+destination+" through underlying portId "
							+sessionId+": "+ cdapMessage.toString());
				}
				
				cdapSessionManager.messageSent(cdapMessage, sessionId);
			}catch(Exception ex){
				if (ex.getMessage() != null && ex.getMessage().equals("Flow closed")){
					cdapSessionManager.removeCDAPSession(sessionId);
				}
				throw new RIBDaemonException(RIBDaemonException.PROBLEMS_SENDING_CDAP_MESSAGE, ex);
			}

			if (cdapMessage.getInvokeID() != 0 && !cdapMessage.getOpCode().equals(Opcode.M_CONNECT) 
					&& !cdapMessage.getOpCode().equals(Opcode.M_RELEASE)
					&& !cdapMessage.getOpCode().equals(Opcode.M_CANCELREAD_R)
					&& !cdapMessage.getOpCode().equals(Opcode.M_CONNECT_R)
					&& !cdapMessage.getOpCode().equals(Opcode.M_CREATE_R) 
					&& !cdapMessage.getOpCode().equals(Opcode.M_READ_R)
					&& !cdapMessage.getOpCode().equals(Opcode.M_DELETE_R)
					&& !cdapMessage.getOpCode().equals(Opcode.M_RELEASE_R)
					&& !cdapMessage.getOpCode().equals(Opcode.M_START_R)
					&& !cdapMessage.getOpCode().equals(Opcode.M_STOP_R)
					&& !cdapMessage.getOpCode().equals(Opcode.M_WRITE_R)){
				messageHandlersWaitingForReply.put(cdapMessage.getInvokeID(), cdapMessageHandler);
			}
		}
	}
	
	/**
	 * Send an information update, consisting on a set of CDAP messages, using the updateStrategy update strategy
	 * (on demand, scheduled)
	 * @param cdapMessages
	 * @param updateStrategy
	 */
	public void sendMessages(CDAPMessage[] cdapMessage, UpdateStrategy arg1) {
		// TODO Auto-generated method stub
	}
	
	/**
	 * Add a RIB object to the RIB
	 * @param ribHandler
	 * @param objectName
	 * @throws RIBDaemonException
	 */
	public void addRIBObject(RIBObject ribObject) throws RIBDaemonException{
		rib.addRIBObject(ribObject);
		log.info("RIBObject with objectname "+ribObject.getObjectName()+", objectClass "+ribObject.getObjectClass()+", " +
				"objectInstance "+ribObject.getObjectInstance()+" and object value " + ribObject.getObjectValue() +
				" added to the RIB");
	}
	
	/**
	 * Remove a ribObject from the RIB
	 * @param objectName
	 * @throws RIBDaemonException
	 */
	public void removeRIBObject(RIBObject ribObject) throws RIBDaemonException{
		if (ribObject != null){
			removeRIBObject(ribObject.getObjectName());
		}else{
			throw new RIBDaemonException(RIBDaemonException.RIB_OBJECT_AND_OBJECT_NAME_NULL, 
					"Both the RIBObject and objectname parameters are null");
		}
		
		
	}
	
	public void removeRIBObject(String objectName) throws RIBDaemonException{
		RIBObject ribObject = null;
		ribObject = rib.removeRIBObject(objectName);
		
		if (ribObject == null){
			throw new RIBDaemonException(RIBDaemonException.OBJECTNAME_NOT_PRESENT_IN_THE_RIB, 
					"Could not find "+objectName+ " in the RIB");
		}
		
		log.info("RIBObject with objectname "+ribObject.getObjectName()+", objectClass "+ribObject.getObjectClass()+", " +
				"objectInstance "+ribObject.getObjectInstance()+" removed from the RIB");
	}
	
	/**
	 * Reads/writes/created/deletes/starts/stops one or more objects at the RIB, matching the information specified by objectId + objectClass or objectInstance.
	 * At least objectName or objectInstance have to be not null. This operation is invoked because the RIB Daemon has received a CDAP message from another 
	 * IPC process
	 * @param cdapMessage The CDAP message received
	 * @param cdapSessionDescriptor Describes the CDAP session to where the CDAP message belongs
	 * @throws RIBDaemonException on a number of circumstances
	 */
	public void processOperation(CDAPMessage cdapMessage, CDAPSessionDescriptor cdapSessionDescriptor) throws RIBDaemonException{
		log.debug("Remote operation "+cdapMessage.getOpCode()+" called on object "+cdapMessage.getObjName());
		RIBObject ribObject = null;
		
		switch(cdapMessage.getOpCode()){
		case M_CREATE:
			/* Creation is delegated to the parent objects if the object doesn't exist. Create semantics are CREATE or UPDATE. If the 
			 * object exists it is an update, therefore the message is handled to the object. If the object doesn't exist it is a CREATE, 
			 * therefore it is handled to the parent object*/
			try{
				ribObject = getRIBObject(cdapMessage.getObjClass(), cdapMessage.getObjInst(), cdapMessage.getObjName());
			}catch(RIBDaemonException ex){
				if(ex.getErrorCode() == RIBDaemonException.OBJECTNAME_NOT_PRESENT_IN_THE_RIB){
					//The object does not exist, call the parent object
					String parentObjectName = cdapMessage.getObjName().substring(0, cdapMessage.getObjName().lastIndexOf(RIBObjectNames.SEPARATOR));
					ribObject = getRIBObject(null, 0, parentObjectName);
				}else{
					throw ex;
				}
			}
			ribObject.create(cdapMessage, cdapSessionDescriptor);
			break;
		case M_DELETE:
			ribObject = getRIBObject(cdapMessage.getObjClass(), cdapMessage.getObjInst(), cdapMessage.getObjName());
			ribObject.delete(cdapMessage, cdapSessionDescriptor);
			break;
		case M_READ:
			ribObject = getRIBObject(cdapMessage.getObjClass(), cdapMessage.getObjInst(), cdapMessage.getObjName());
			ribObject.read(cdapMessage, cdapSessionDescriptor);
			break;
		case M_CANCELREAD:
			ribObject = getRIBObject(cdapMessage.getObjClass(), cdapMessage.getObjInst(), cdapMessage.getObjName());
			ribObject.cancelRead(cdapMessage, cdapSessionDescriptor);
			break;
		case M_WRITE:
			ribObject = getRIBObject(cdapMessage.getObjClass(), cdapMessage.getObjInst(), cdapMessage.getObjName());
			ribObject.write(cdapMessage, cdapSessionDescriptor);
			break;
		case M_START:
			ribObject = getRIBObject(cdapMessage.getObjClass(), cdapMessage.getObjInst(), cdapMessage.getObjName());
			ribObject.start(cdapMessage, cdapSessionDescriptor);
			break;
		case M_STOP:
			ribObject = getRIBObject(cdapMessage.getObjClass(), cdapMessage.getObjInst(), cdapMessage.getObjName());
			ribObject.stop(cdapMessage, cdapSessionDescriptor);
			break;
		default:
			throw new RIBDaemonException(RIBDaemonException.OPERATION_NOT_ALLOWED_AT_THIS_OBJECT);
		}
	}
	
	private RIBObject getRIBObject(String objectClass, long objectInstance, String objectName) throws RIBDaemonException{
		validateObjectArguments(objectClass, objectInstance, objectName);
		return rib.getRIBObject(objectName);
	}

	/**
	 * Create or update an object in the RIB
	 * @param objectClass the class of the object
	 * @param objectName the name of the object
	 * @param objectInstance the instance of the object
	 * @param objectValue the value of the object
	 * @param notify if not null notify some of the neighbors about the change
	 * @throws RIBDaemonException
	 */
	@Override
	public void create(String objectClass, long objectInstance, String objectName, 
			Object objectValue, NotificationPolicy notificationPolicy) throws RIBDaemonException {
		log.debug("Local operation create called on object "+objectName);
		validateObjectArguments(objectClass, objectInstance, objectName);

		RIBObject ribObject = null;
		try{
			ribObject = getRIBObject(objectClass, objectInstance, objectName);
		}catch(RIBDaemonException ex){
			/* Creation is delegated to the parent objects if the object doesn't exist*/
			String parentObjectName = objectName.substring(0, objectName.lastIndexOf(RIBObjectNames.SEPARATOR));
			ribObject = getRIBObject(null, 0, parentObjectName);
		}

		//Create the object
		ribObject.create(objectClass, objectInstance, objectName, objectValue);
		
		//If we don't need to notify, we're done
		if (notificationPolicy == null){
			return;
		}
		
		//We need to notify, find out to whom the notifications must be sent to, and do it
		int[] peersNotToNotify = notificationPolicy.getCdapSessionIds();
		int[] peers = cdapSessionManager.getAllCDAPSessionIds();
		ObjectValue cdapObjectValue = new ObjectValue();
		
		try{
			cdapObjectValue.setByteval(encoder.encode(objectValue));
		}catch(Exception ex){
			log.error("Could not send notification of create object to remote peers because: "+ex.getMessage());
			return;
		}
		
		CDAPMessage cdapMessage = null;
		for(int i=0; i<peers.length; i++){
			if (!isOnList(peers[i], peersNotToNotify)){
				try{
					cdapMessage = cdapSessionManager.getCreateObjectRequestMessage(peers[i], null, null, 
							objectClass, objectInstance, objectName, cdapObjectValue, 0, false);
					this.sendMessage(cdapMessage, peers[i], null);
				}catch(Exception ex){
					log.error(ex);
				}
			}
		}
	}
	
	/**
	 * Finds out of the candidate number is on the list
	 * @param candidate
	 * @param list
	 * @return
	 */
	private boolean isOnList(int candidate, int[] list){
		for(int i=0; i<list.length; i++){
			if (list[i] == candidate){
				return true;
			}
		}
		
		return false;
	}

	@Override
	public void delete(String objectClass, long objectInstance, String objectName, 
			Object objectValue, NotificationPolicy notificationPolicy) throws RIBDaemonException {
		log.debug("Local operation delete called on object "+objectName);
		validateObjectArguments(objectClass, objectInstance, objectName);
		RIBObject ribObject = getRIBObject(objectClass, objectInstance, objectName);
		ribObject.delete(objectValue);
		
		//If we don't need to notify, we're done
		if (notificationPolicy == null){
			return;
		}
		
		//We need to notify, find out to whom the notifications must be sent to, and do it
		int[] peersNotToNotify = notificationPolicy.getCdapSessionIds();
		int[] peers = cdapSessionManager.getAllCDAPSessionIds();
		ObjectValue cdapObjectValue = null;
		
		if (objectValue != null){
			try{
				cdapObjectValue = new ObjectValue();
				cdapObjectValue.setByteval(encoder.encode(objectValue));
			}catch(Exception ex){
				log.error("Could not send notification of create object to remote peers because: "+ex.getMessage());
				return;
			}
		}
		
		CDAPMessage cdapMessage = null;
		for(int i=0; i<peers.length; i++){
			if (!isOnList(peers[i], peersNotToNotify)){
				try{
					cdapMessage = cdapSessionManager.getDeleteObjectRequestMessage(peers[i], null, null, 
							objectClass, objectInstance, objectName, cdapObjectValue, 0, false);
					this.sendMessage(cdapMessage, peers[i], null);
				}catch(Exception ex){
					log.error(ex);
				}
			}
		}
	}

	@Override
	public RIBObject read(String objectClass, long objectInstance, String objectName) throws RIBDaemonException {
		validateObjectArguments(objectClass, objectInstance, objectName);
		return getRIBObject(objectClass, objectInstance, objectName);
	}

	@Override
	public void start(String objectClass, long objectInstance, String objectName, Object object) throws RIBDaemonException {
		log.debug("Local operation start called on object "+objectName);
		validateObjectArguments(objectClass, objectInstance, objectName);
		RIBObject ribObject = getRIBObject(objectClass, objectInstance, objectName);
		ribObject.start(object);
	}

	@Override
	public void stop(String objectClass, long objectInstance, String objectName, Object object) throws RIBDaemonException {
		log.debug("Local operation stop called on object "+objectName);
		validateObjectArguments(objectClass, objectInstance, objectName);
		RIBObject ribObject = getRIBObject(objectClass, objectInstance, objectName);
		ribObject.stop(object);
	}

	/**
	 * Store an object to the RIB. This may cause a new object to be created in the RIB, or an existing object to be updated.
	 * @param objectClass optional if objectInstance specified, mandatory otherwise. A string identifying the class of the object
	 * @param objectInstance objectInstance optional if objectClass specified, mandatory otherwise. An id that uniquely identifies the object within a RIB
	 * @param objectName optional if objectClass specified, ignored otherwise. An id that uniquely identifies an object within an objectClass
	 * @param objectToWrite the object to be written to the RIB
	 * @throws RIBDaemonException if there are problems performing the "write" operation to the RIB
	 */
	@Override
	public void write(String objectClass, long objectInstance, String objectName, 
			Object object, NotificationPolicy notification) throws RIBDaemonException {
		log.debug("Local operation write called on object "+objectName);
		validateObjectArguments(objectClass, objectInstance, objectName);
		RIBObject ribObject = getRIBObject(objectClass, objectInstance, objectName);
		ribObject.write(object);
	}
	
	/**
	 * At least objectName must not be null or objectInstance != from -1
	 * @param objectClass
	 * @param objectName
	 * @param objectInstance
	 * @throws RIBDaemonException
	 */
	private void validateObjectArguments(String objectClass, long objectInstance, String objectName) throws RIBDaemonException{
		if (objectName == null){
			throw new RIBDaemonException(RIBDaemonException.OBJECTCLASS_AND_OBJECT_NAME_OR_OBJECT_INSTANCE_NOT_SPECIFIED);
		}
	}

	@Override
	public List<RIBObject> getRIBObjects() {
		return rib.getRIBObjects();
	}
	
	/**
	 * Process a Query RIB Request from the IPC Manager
	 * @param event
	 */
	public void processQueryRIBRequestEvent(QueryRIBRequestEvent event) {
		List<RIBObject> ribObjects = getRIBObjects();
		RIBObjectList list = new RIBObjectList();
		for(int i=0; i<ribObjects.size(); i++) {
			list.addLast(ribObjects.get(i).toLibrinaRIBObject());
		}
		
		try {
			rina.getExtendedIPCManager().queryRIBResponse(event, 0, list);
		} catch (Exception ex) {
			log.error("Problems sending Query RIB Response to IPC Manager: " + ex.getMessage());
		}
	}
}
