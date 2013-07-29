package rina.cdap.impl;

import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentMap;

import rina.cdap.api.CDAPException;
import rina.cdap.api.CDAPMessageValidator;
import rina.cdap.api.CDAPSession;
import rina.cdap.api.CDAPSessionDescriptor;
import rina.cdap.api.CDAPSessionInvokeIdManager;
import rina.cdap.api.CDAPSessionManager;
import rina.cdap.api.message.CDAPMessage;
import rina.cdap.api.message.CDAPMessage.Flags;
import rina.cdap.api.message.CDAPMessage.Opcode;

/**
 * Implements a CDAP session. Has the necessary logic to ensure that a 
 * the operation of the CDAP protocol is consistent (correct states and proper
 * invocation of operations)
 * @author eduardgrasa
 *
 */
public class CDAPSessionImpl implements CDAPSession{
	
	/**
	 * Deals with the connection establishment and deletion messages and states
	 */
	private ConnectionStateMachine connectionStateMachine = null;
	
	/**
	 * This map contains the invokeIds of the messages that
	 * have requested a response, except for the M_CANCELREADs
	 */
	private ConcurrentMap<Integer, CDAPOperationState> pendingMessages = null;
	
	private ConcurrentMap<Integer, CDAPOperationState> cancelReadPendingMessages = null;
	
	private WireMessageProvider wireMessageProvider = null;
	
	private CDAPSessionDescriptor sessionDescriptor = null;
	
	private CDAPSessionManager cdapSessionManager = null;
	
	private CDAPSessionInvokeIdManager invokeIdManager = null;
	
	public CDAPSessionImpl(CDAPSessionManager cdapSessionManager, long timeout){
		this.cdapSessionManager = cdapSessionManager;
		this.invokeIdManager = new CDAPSessionInvokeIdManagerImpl();
		pendingMessages = new ConcurrentHashMap<Integer, CDAPOperationState>();
		this.cancelReadPendingMessages = new ConcurrentHashMap<Integer, CDAPOperationState>();
		this.connectionStateMachine = new ConnectionStateMachine(this, timeout);
	}
	
	public CDAPSessionInvokeIdManager getInvokeIdManager(){
		return invokeIdManager;
	}
	
	public void setWireMessageProvider(WireMessageProvider wireMessageProvider){
		this.wireMessageProvider = wireMessageProvider;
	}
	
	protected void stopConnection(){
		pendingMessages.clear();
		cancelReadPendingMessages.clear();
		cdapSessionManager.removeCDAPSession(this.getPortId());
	}
	
	public boolean isConnected(){
		return connectionStateMachine.isConnected();
	}
	
	public byte[] encodeNextMessageToBeSent(CDAPMessage cdapMessage) throws CDAPException{
		CDAPMessageValidator.validate(cdapMessage);

		switch(cdapMessage.getOpCode()){
			case M_CONNECT:
				connectionStateMachine.checkConnect();
				break;
			case M_CONNECT_R:
				connectionStateMachine.checkConnectResponse();
				break;
			case M_RELEASE:
				connectionStateMachine.checkRelease();
				break;
			case M_RELEASE_R:
				connectionStateMachine.checkReleaseResponse();
				break;
			case M_CREATE:
				checkIsConnected();
				checkInvokeIdNotExists(cdapMessage);
				break;
			case M_CREATE_R:
				checkIsConnected();
				checkCanSendOrReceiveResponse(cdapMessage, Opcode.M_CREATE, true);
				break;
			case M_DELETE:
				checkIsConnected();
				checkInvokeIdNotExists(cdapMessage);
				break;
			case M_DELETE_R:
				checkIsConnected();
				checkCanSendOrReceiveResponse(cdapMessage, Opcode.M_DELETE, true);
				break;
			case M_START:
				checkIsConnected();
				checkInvokeIdNotExists(cdapMessage);
				break;
			case M_START_R:
				checkIsConnected();
				checkCanSendOrReceiveResponse(cdapMessage, Opcode.M_START, true);
				break;
			case M_STOP:
				checkIsConnected();
				checkInvokeIdNotExists(cdapMessage);
				break;
			case M_STOP_R:
				checkIsConnected();
				checkCanSendOrReceiveResponse(cdapMessage, Opcode.M_STOP, true);
				break;
			case M_WRITE:
				checkIsConnected();
				checkInvokeIdNotExists(cdapMessage);
				break;
			case M_WRITE_R:
				checkIsConnected();
				checkCanSendOrReceiveResponse(cdapMessage, Opcode.M_WRITE, true);
				break;
			case M_READ:
				checkIsConnected();
				checkInvokeIdNotExists(cdapMessage);
				break;
			case M_READ_R:
				checkIsConnected();
				checkCanSendOrReceiveResponse(cdapMessage, Opcode.M_READ, true);
				break;
			case M_CANCELREAD:
				checkIsConnected();
				checkCanSendOrReceiveCancelReadRequest(cdapMessage, true);
				break;
			case M_CANCELREAD_R:
				checkIsConnected();
				checkCanSendOrReceiveCancelReadResponse(cdapMessage, true);
				break;
			default:
				throw new CDAPException("Unrecognized operation code: "+cdapMessage.getOpCode().toString());	
		}
		
		return serializeMessage(cdapMessage);
	}
	
	public void messageSent(CDAPMessage cdapMessage) throws CDAPException{
		messageSentOrReceived(cdapMessage, true);
	}
	
	public CDAPMessage messageReceived(byte[] message) throws CDAPException{
		CDAPMessage cdapMessage = deserializeMessage(message);
		return messageSentOrReceived(cdapMessage, false);
	}
	
	public CDAPMessage messageReceived(CDAPMessage cdapMessage) throws CDAPException {
		return messageSentOrReceived(cdapMessage, false);
	}
	
	private CDAPMessage messageSentOrReceived(CDAPMessage cdapMessage, boolean sent) throws CDAPException{
		switch(cdapMessage.getOpCode()){
		case M_CONNECT:
			connectionStateMachine.connectSentOrReceived(cdapMessage, sent);
			populateSessionDescriptor(cdapMessage, sent);
			break;
		case M_CONNECT_R:
			connectionStateMachine.connectResponseSentOrReceived(cdapMessage, sent);
			break;
		case M_RELEASE:
			connectionStateMachine.releaseSentOrReceived(cdapMessage, sent);
			break;
		case M_RELEASE_R:
			connectionStateMachine.releaseResponseSentOrReceived(cdapMessage, sent);
			emptySessionDescriptor();
			break;
		case M_CREATE:
			requestMessageSentOrReceived(cdapMessage, Opcode.M_CREATE, sent);
			break;
		case M_CREATE_R:
			responseMessageSentOrReceived(cdapMessage, Opcode.M_CREATE, sent);
			break;
		case M_DELETE:
			requestMessageSentOrReceived(cdapMessage, Opcode.M_DELETE, sent);
			break;
		case M_DELETE_R:
			responseMessageSentOrReceived(cdapMessage, Opcode.M_DELETE, sent);
			break;
		case M_START:
			requestMessageSentOrReceived(cdapMessage, Opcode.M_START, sent);
			break;
		case M_START_R:
			responseMessageSentOrReceived(cdapMessage, Opcode.M_START, sent);
			break;
		case M_STOP:
			requestMessageSentOrReceived(cdapMessage, Opcode.M_STOP, sent);
			break;
		case M_STOP_R:
			responseMessageSentOrReceived(cdapMessage, Opcode.M_STOP, sent);
			break;
		case M_WRITE:
			requestMessageSentOrReceived(cdapMessage, Opcode.M_WRITE, sent);
			break;
		case M_WRITE_R:
			responseMessageSentOrReceived(cdapMessage, Opcode.M_WRITE, sent);
			break;
		case M_READ:
			requestMessageSentOrReceived(cdapMessage, Opcode.M_READ, sent);
			break;
		case M_READ_R:
			responseMessageSentOrReceived(cdapMessage, Opcode.M_READ, sent);
			break;
		case M_CANCELREAD:
			cancelReadMessageSentOrReceived(cdapMessage, sent);
			break;
		case M_CANCELREAD_R:
			cancelReadResponseMessageSentOrReceived(cdapMessage, sent);
			break;
		default:
			throw new CDAPException("Unrecognized operation code: "+cdapMessage.getOpCode().toString());	
		}

		CDAPMessageValidator.validate(cdapMessage);
		
		freeOrReserveInvokeId(cdapMessage);
		
		return cdapMessage;
	}
	
	private void freeOrReserveInvokeId(CDAPMessage cdapMessage){
		Opcode opcode = cdapMessage.getOpCode();
		if (opcode.equals(Opcode.M_CONNECT_R) || opcode.equals(Opcode.M_RELEASE_R) || opcode.equals(Opcode.M_CREATE_R) || opcode.equals(Opcode.M_DELETE_R) 
				|| opcode.equals(Opcode.M_START_R) || opcode.equals(Opcode.M_STOP_R) || opcode.equals(Opcode.M_WRITE_R) || opcode.equals(Opcode.M_CANCELREAD_R) || 
				(opcode.equals(Opcode.M_READ_R) && (cdapMessage.getFlags() == null || !cdapMessage.getFlags().equals(Flags.F_RD_INCOMPLETE)))){
			invokeIdManager.freeInvokeId(new Integer(cdapMessage.getInvokeID()));
		}
		
		if (cdapMessage.getInvokeID() != 0){
			if (opcode.equals(Opcode.M_CONNECT) || opcode.equals(Opcode.M_RELEASE) || opcode.equals(Opcode.M_CREATE) || opcode.equals(Opcode.M_DELETE) 
					|| opcode.equals(Opcode.M_START) || opcode.equals(Opcode.M_STOP) || opcode.equals(Opcode.M_WRITE) || opcode.equals(Opcode.M_CANCELREAD) 
					|| opcode.equals(Opcode.M_READ)){
				invokeIdManager.reserveInvokeId(new Integer(cdapMessage.getInvokeID()));
			}
		}
	}
	
	private void checkIsConnected() throws CDAPException{
		if (!connectionStateMachine.isConnected()){
			throw new CDAPException("Cannot send a message " +
			"because the CDAP session is not in CONNECTED state");
		}
	}
	
	private void checkInvokeIdNotExists(CDAPMessage cdapMessage) throws CDAPException{
		if (pendingMessages.containsKey(new Integer(cdapMessage.getInvokeID()))){
			throw new CDAPException("The invokeid "+cdapMessage.getInvokeID()+" is already in use");
		}
	}
	
	private void checkCanSendOrReceiveCancelReadRequest(CDAPMessage cdapMessage, boolean sender) throws CDAPException{
		boolean validationFailed = false;
		CDAPOperationState state = null;
		
		state = pendingMessages.get(new Integer(cdapMessage.getInvokeID()));
		
		if (state == null){
			throw new CDAPException("Cannot set an M_CANCELREAD message because there is no READ transaction "
					+ "associated to the invoke id "+cdapMessage.getInvokeID());
		}
		if (!state.getOpcode().equals(Opcode.M_READ)){
			validationFailed = true;
		}
		if (sender && !state.isSender()){
			validationFailed = true;
		}
		if (!sender && state.isSender()){
			validationFailed = true;
		}
		if (validationFailed){
			throw new CDAPException("Cannot set an M_CANCELREAD message because there is no READ transaction "
					+ "associated to the invoke id "+cdapMessage.getInvokeID());
		}
	}
	
	private void requestMessageSentOrReceived(CDAPMessage cdapMessage, Opcode opcode, boolean sent) throws CDAPException{
		checkIsConnected();
		checkInvokeIdNotExists(cdapMessage);
		if (cdapMessage.getInvokeID() != 0){
			pendingMessages.put(
					new Integer(cdapMessage.getInvokeID()), 
					new CDAPOperationState(opcode, sent));
		}
	}
	
	private void cancelReadMessageSentOrReceived(CDAPMessage cdapMessage, boolean sender) throws CDAPException{
		checkCanSendOrReceiveCancelReadRequest(cdapMessage, sender);
		cancelReadPendingMessages.put(
				new Integer(cdapMessage.getInvokeID()), 
				new CDAPOperationState(Opcode.M_CANCELREAD, sender));
	}
	
	private void checkCanSendOrReceiveResponse(CDAPMessage cdapMessage, Opcode opcode, boolean send) throws CDAPException{
		boolean validationFailed = false;
		CDAPOperationState state = null;
		
		state = pendingMessages.get(new Integer(cdapMessage.getInvokeID()));
		
		if (state == null){
			throw new CDAPException("Cannot send a response for the "+opcode+
					" operation with invokeId "+ cdapMessage.getInvokeID());
		}
		
		if (!state.getOpcode().equals(opcode)){
			validationFailed = true;
		}
		
		if (send && state.isSender()){
			validationFailed = true;
		}
		
		if (!send && !state.isSender()){
			validationFailed = true;
		}
		
		if (validationFailed){
			throw new CDAPException("Cannot send a response for the "+opcode+
					" operation with invokeId "+ cdapMessage.getInvokeID());
		}
	}
	
	private void checkCanSendOrReceiveCancelReadResponse(CDAPMessage cdapMessage, boolean send) throws CDAPException{
		boolean validationFailed = false;
		CDAPOperationState state = null;
		
		state = cancelReadPendingMessages.get(new Integer(cdapMessage.getInvokeID()));
		
		if (state == null){
			throw new CDAPException("Cannot send a response for the "+Opcode.M_CANCELREAD+
					" operation with invokeId "+ cdapMessage.getInvokeID());
		}
		
		if (!state.getOpcode().equals(Opcode.M_CANCELREAD)){
			validationFailed = true;
		}
		
		if (send && state.isSender()){
			validationFailed = true;
		}
		
		if (!send && !state.isSender()){
			validationFailed = true;
		}
		
		if (validationFailed){
			throw new CDAPException("Cannot send a response for the "+Opcode.M_CANCELREAD+
					" operation with invokeId "+ cdapMessage.getInvokeID());
		}
	}
	
	private void responseMessageSentOrReceived(CDAPMessage cdapMessage, Opcode opcode, boolean sent) throws CDAPException{
		checkIsConnected();
		checkCanSendOrReceiveResponse(cdapMessage, opcode, sent);
		boolean operationComplete = true;
		if (opcode.equals(Opcode.M_READ)){
			Flags flags = cdapMessage.getFlags();
			if (flags != null && flags.equals(Flags.F_RD_INCOMPLETE)){
					operationComplete = false;
			}
		}
		if (operationComplete){
			pendingMessages.remove(new Integer(cdapMessage.getInvokeID()));
		}
		
		//check for M_READ_R and M_CANCELREAD race condition
		if (!sent){
			if (opcode.equals(Opcode.M_READ)){
				cancelReadPendingMessages.remove(cdapMessage.getInvokeID());
			}
		}
	}
	
	private void cancelReadResponseMessageSentOrReceived(CDAPMessage cdapMessage, boolean sent) throws CDAPException{
		checkIsConnected();
		checkCanSendOrReceiveCancelReadResponse(cdapMessage, sent);
		cancelReadPendingMessages.remove(new Integer(cdapMessage.getInvokeID()));
		pendingMessages.remove(new Integer(cdapMessage.getInvokeID()));
	}
	
	private byte[] serializeMessage(CDAPMessage cdapMessage) throws CDAPException{
		return wireMessageProvider.serializeMessage(cdapMessage);
	}

	private CDAPMessage deserializeMessage(byte[] message) throws CDAPException{
		return wireMessageProvider.deserializeMessage(message);
	}

	public void setSessionDescriptor(CDAPSessionDescriptor sessionDescriptor) {
		this.sessionDescriptor = sessionDescriptor;
	}

	public CDAPSessionDescriptor getSessionDescriptor() {
		return sessionDescriptor;
	}

	public int getPortId() {
		return sessionDescriptor.getPortId();
	}
	
	private void populateSessionDescriptor(CDAPMessage cdapMessage, boolean send){
		sessionDescriptor.setAbsSyntax(cdapMessage.getAbsSyntax());
		sessionDescriptor.setAuthMech(cdapMessage.getAuthMech());
		sessionDescriptor.setAuthValue(cdapMessage.getAuthValue());
		if (send){
			sessionDescriptor.setDestAEInst(cdapMessage.getDestAEInst());
			sessionDescriptor.setDestAEName(cdapMessage.getDestAEName());
			sessionDescriptor.setDestApInst(cdapMessage.getDestApInst());
			sessionDescriptor.setDestApName(cdapMessage.getDestApName());
			sessionDescriptor.setSrcAEInst(cdapMessage.getSrcAEInst());
			sessionDescriptor.setSrcAEName(cdapMessage.getSrcAEName());
			sessionDescriptor.setSrcApInst(cdapMessage.getSrcApInst());
			sessionDescriptor.setSrcApName(cdapMessage.getSrcApName());
		}else{
			sessionDescriptor.setDestAEInst(cdapMessage.getSrcAEInst());
			sessionDescriptor.setDestAEName(cdapMessage.getSrcAEName());
			sessionDescriptor.setDestApInst(cdapMessage.getSrcApInst());
			sessionDescriptor.setDestApName(cdapMessage.getSrcApName());
			sessionDescriptor.setSrcAEInst(cdapMessage.getDestAEInst());
			sessionDescriptor.setSrcAEName(cdapMessage.getDestAEName());
			sessionDescriptor.setSrcApInst(cdapMessage.getDestApInst());
			sessionDescriptor.setSrcApName(cdapMessage.getDestApName());
		}
		sessionDescriptor.setVersion(cdapMessage.getVersion());
	}
	
	private void emptySessionDescriptor(){
		sessionDescriptor.setAbsSyntax(-1);
		sessionDescriptor.setAuthMech(null);
		sessionDescriptor.setAuthValue(null);
		sessionDescriptor.setDestAEInst(null);
		sessionDescriptor.setDestAEName(null);
		sessionDescriptor.setDestApInst(null);
		sessionDescriptor.setDestApName(null);
		sessionDescriptor.setSrcAEInst(null);
		sessionDescriptor.setSrcAEName(null);
		sessionDescriptor.setSrcApInst(null);
		sessionDescriptor.setSrcApName(null);
		sessionDescriptor.setVersion(-1);
	}
}