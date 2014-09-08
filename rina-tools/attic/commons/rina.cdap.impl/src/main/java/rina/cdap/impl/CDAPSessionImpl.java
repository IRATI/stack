package rina.cdap.impl;

import rina.cdap.api.CDAPException;
import rina.cdap.api.CDAPInvokeIdManager;
import rina.cdap.api.CDAPMessageValidator;
import rina.cdap.api.CDAPSession;
import rina.cdap.api.CDAPSessionDescriptor;
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
	
	private WireMessageProvider wireMessageProvider = null;
	
	private CDAPSessionDescriptor sessionDescriptor = null;
	
	private CDAPSessionManager cdapSessionManager = null;
	
	private CDAPInvokeIdManager invokeIdManager = null;
	
	public CDAPSessionImpl(CDAPSessionManager cdapSessionManager, long timeout, 
			CDAPInvokeIdManager invokeIdManager){
		this.cdapSessionManager = cdapSessionManager;
		this.invokeIdManager = invokeIdManager;
		this.connectionStateMachine = new ConnectionStateMachine(this, timeout);
	}
	
	public CDAPInvokeIdManager getInvokeIdManager(){
		return invokeIdManager;
	}
	
	public void setWireMessageProvider(WireMessageProvider wireMessageProvider){
		this.wireMessageProvider = wireMessageProvider;
	}
	
	protected void stopConnection(){
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
				break;
			case M_CREATE_R:
				checkIsConnected();
				break;
			case M_DELETE:
				checkIsConnected();
				break;
			case M_DELETE_R:
				checkIsConnected();
				break;
			case M_START:
				checkIsConnected();
				break;
			case M_START_R:
				checkIsConnected();
				break;
			case M_STOP:
				checkIsConnected();
				break;
			case M_STOP_R:
				checkIsConnected();
				break;
			case M_WRITE:
				checkIsConnected();
				break;
			case M_WRITE_R:
				checkIsConnected();
				break;
			case M_READ:
				checkIsConnected();
				break;
			case M_READ_R:
				checkIsConnected();
				break;
			case M_CANCELREAD:
				checkIsConnected();;
				break;
			case M_CANCELREAD_R:
				checkIsConnected();
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
	
	private void requestMessageSentOrReceived(CDAPMessage cdapMessage, Opcode opcode, boolean sent) throws CDAPException{
		checkIsConnected();
	}
	
	private void responseMessageSentOrReceived(CDAPMessage cdapMessage, Opcode opcode, boolean sent) throws CDAPException{
		checkIsConnected();
	}
	
	private void cancelReadResponseMessageSentOrReceived(CDAPMessage cdapMessage, boolean sent) throws CDAPException{
		checkIsConnected();
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