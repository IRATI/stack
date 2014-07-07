package rina.cdap.impl;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.Map.Entry;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentMap;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import rina.cdap.api.CDAPException;
import rina.cdap.api.CDAPSession;
import rina.cdap.api.CDAPSessionDescriptor;
import rina.cdap.api.CDAPSessionManager;
import rina.cdap.api.message.AuthValue;
import rina.cdap.api.message.CDAPMessage;
import rina.cdap.api.message.ObjectValue;
import rina.cdap.api.message.CDAPMessage.AuthTypes;
import rina.cdap.api.message.CDAPMessage.Flags;
import rina.cdap.api.message.CDAPMessage.Opcode;

public class CDAPSessionManagerImpl implements CDAPSessionManager{
	
	public static final long DEFAULT_TIMEOUT_IN_MS = 10000;
	
	private static final Log log = LogFactory.getLog(CDAPSessionManagerImpl.class);
	
	private WireMessageProviderFactory wireMessageProviderFactory = null;
	
	private ConcurrentMap<Integer, CDAPSession> cdapSessions = null;
	
	/**
	 * Used by the serialize and unserialize operations
	 */
	private WireMessageProvider wireMessageProvider =  null;
	
	/**
	 * The maximum time the CDAP state machine of a session will wait for connect or release responses (in ms)
	 */
	private long timeout = 0;
	
	private CDAPInvokeIdManagerImpl invokeIdManager = null;
	
	public CDAPSessionManagerImpl(WireMessageProviderFactory wireMessageProviderFactory){
		this.cdapSessions = new ConcurrentHashMap<Integer, CDAPSession>();
		this.wireMessageProviderFactory = wireMessageProviderFactory;
		this.invokeIdManager = new CDAPInvokeIdManagerImpl();
		timeout = DEFAULT_TIMEOUT_IN_MS;
	}

	public CDAPSession createCDAPSession(int portId) {
		CDAPSessionImpl cdapSession = new CDAPSessionImpl(this, timeout, invokeIdManager);
		cdapSession.setWireMessageProvider(wireMessageProviderFactory.createWireMessageProvider());
		CDAPSessionDescriptor descriptor = new CDAPSessionDescriptor();
		descriptor.setPortId(portId);
		cdapSession.setSessionDescriptor(descriptor);
		CDAPSession existingCDAPSession = 
			cdapSessions.putIfAbsent(new Integer(descriptor.getPortId()), cdapSession);
		if (existingCDAPSession == null){
			return cdapSession;
		}else{
			return existingCDAPSession;
		}
	}
	
	private WireMessageProvider getWireMessageProvider(){
		synchronized(this){
			if (this.wireMessageProvider == null){
				this.wireMessageProvider = this.wireMessageProviderFactory.createWireMessageProvider();
			}
		}
		
		return this.wireMessageProvider;
	}
	
	/**
	 * Get the identifiers of all the CDAP sessions
	 * @return
	 */
	public int[] getAllCDAPSessionIds(){
		Set<Integer> keySet = null;
		
		synchronized(this){
			keySet = cdapSessions.keySet();
		}
		
		int[] result = new int[keySet.size()];
		
		int i=0;
		Iterator<Integer> iterator = keySet.iterator();
		while(iterator.hasNext()){
			result[i] = iterator.next();
			i++;
		}
		
		return result;
	}

	public List<CDAPSession> getAllCDAPSessions() {
		Iterator<Entry<Integer, CDAPSession>> iterator = null;
		
		synchronized(this){
			iterator =  cdapSessions.entrySet().iterator();
		}
		
		List<CDAPSession> result = new ArrayList<CDAPSession>();
		while(iterator.hasNext()){
			result.add(iterator.next().getValue());
		}
		
		return result;
	}

	public CDAPSession getCDAPSession(int portId) {
		return cdapSessions.get(new Integer(portId));
	}
	
	/**
	 * Encodes a CDAP message. It just converts a CDAP message into a byte 
	 * array, without caring about what session this CDAP message belongs to (and 
	 * therefore it doesn't update any CDAP session state machine). Called by 
	 * functions that have to relay CDAP messages, and need to encode/
	 * decode its contents to make the relay decision and maybe modify some 
	 * message values.
	 * @param cdapMessage
	 * @return
	 * @throws CDAPException
	 */
	public byte[] encodeCDAPMessage(CDAPMessage cdapMessage) throws CDAPException{
		return getWireMessageProvider().serializeMessage(cdapMessage);
	}
	
	/**
	 * Decodes a CDAP message. It just converts the byte array into a CDAP 
	 * message, without caring about what session this CDAP message belongs to (and 
	 * therefore it doesn't update any CDAP session state machine). Called by 
	 * functions that have to relay CDAP messages, and need to encode/
	 * decode its contents to make the relay decision and maybe modify some 
	 * @param cdapMessage
	 * @return
	 * @throws CDAPException
	 */
	public CDAPMessage decodeCDAPMessage(byte[] cdapMessage) throws CDAPException{
		return getWireMessageProvider().deserializeMessage(cdapMessage);
	}
	
	/**
	 * Called by the CDAPSession state machine when the cdap session is terminated
	 * @param portId
	 */
	public void removeCDAPSession(int portId){
		cdapSessions.remove(new Integer(portId));
	}


	/**
	 * Encodes the next CDAP message to be sent, and checks against the 
	 * CDAP state machine that this is a valid message to be sent
	 * @param cdapMessage The cdap message to be serialized
	 * @param portId 
	 * @return encoded version of the CDAP Message
	 * @throws CDAPException
	 */
	public byte[] encodeNextMessageToBeSent(CDAPMessage cdapMessage, int portId) throws CDAPException {
		CDAPSession cdapSession = this.getCDAPSession(portId);

		if (cdapSession == null && cdapMessage.getOpCode() == Opcode.M_CONNECT){
			cdapSession = this.createCDAPSession(portId);
		}else if (cdapSession == null && cdapMessage.getOpCode() != Opcode.M_CONNECT){
			throw new CDAPException("There are no open CDAP sessions associated to the flow identified by "+portId+" right now");
		}
			
		return cdapSession.encodeNextMessageToBeSent(cdapMessage);
	}

	/**
	 * Depending on the message received, it will create a new CDAP state machine (CDAP Session), or update 
	 * an existing one, or terminate one.
	 * @param encodedCDAPMessage
	 * @param portId
	 * @return Decoded CDAP Message
	 * @throws CDAPException if the message is not consistent with the appropriate CDAP state machine
	 */
	public CDAPMessage messageReceived(byte[] encodedCDAPMessage, int portId) throws CDAPException {
		CDAPMessage cdapMessage = this.decodeCDAPMessage(encodedCDAPMessage);
		CDAPSession cdapSession = this.getCDAPSession(portId);
		
		if (cdapSession != null){
			log.debug("Received CDAP message from "+ cdapSession.getSessionDescriptor().getDestApName()+ 
				" through underlying portId "+portId+". Decoded contents: "+cdapMessage.toString());
		}else{
			log.debug("Received CDAP message from "+ cdapMessage.getDestApName()+ 
					" through underlying portId "+portId+". Decoded contents: "+cdapMessage.toString());
		}
		
		switch(cdapMessage.getOpCode()){
		case M_CONNECT:
			if (cdapSession == null){
				cdapSession = this.createCDAPSession(portId);
				cdapSession.messageReceived(cdapMessage);
				log.debug("Created a new CDAP session for port "+portId);
			}else{
				throw new CDAPException("M_CONNECT received on an already open CDAP Session, over flow " + portId);
			}
			break;
		default:
			if (cdapSession != null){
				cdapSession.messageReceived(cdapMessage);
			}else{
				throw new CDAPException("Receive a "+cdapMessage.getOpCode()+" CDAP message on a CDAP session that is not open, over flow "+portId);
			}
			break;
		}
		
		return cdapMessage;
	}

	/**
	 * Update the CDAP state machine because we've sent a message through the
	 * flow identified by portId
	 * @param cdapMessage The CDAP message to be serialized
	 * @param portId 
	 * @return encoded version of the CDAP Message
	 * @throws CDAPException
	 */
	public void messageSent(CDAPMessage cdapMessage, int portId) throws CDAPException {
		CDAPSession cdapSession = this.getCDAPSession(portId);
		if (cdapSession == null && cdapMessage.getOpCode() == Opcode.M_CONNECT){
			cdapSession = this.createCDAPSession(portId);
		}else if (cdapSession == null && cdapMessage.getOpCode() != Opcode.M_CONNECT){
			throw new CDAPException("There are no open CDAP sessions associated to the flow identified by "+portId+" right now");
		}
			
		cdapSession.messageSent(cdapMessage);
	}
	
	/**
	 * Return the portId of the (N-1) Flow that supports the CDAP Session
	 * with the IPC process identified by destinationApplicationProcessName
	 * @param destinationApplicationProcessName
	 * @throws CDAPException
	 */
	public int getPortId(String destinationApplicationProcessName) throws CDAPException{
		Iterator<Entry<Integer, CDAPSession>> iterator = null;
		iterator = this.cdapSessions.entrySet().iterator();
		
		CDAPSession currentSession = null;
		while(iterator.hasNext()){
			currentSession = iterator.next().getValue();
			if (currentSession.getSessionDescriptor().getDestApName().equals(destinationApplicationProcessName)){
					return currentSession.getPortId();
			}
		}
		
		throw new CDAPException("Don't have a running CDAP sesion to "+ destinationApplicationProcessName);
	}
	
	/**
	 * Create a M_CONNECT CDAP Message
	 * @param portId identifies the CDAP Session that this message is part of
	 * @param authMech
	 * @param authValue
	 * @param destAEInst
	 * @param destAEName
	 * @param destApInst
	 * @param destApName
	 * @param srcAEInst
	 * @param srcAEName
	 * @param srcApInst
	 * @param srcApName
	 * @param invokeId true if one is requested, false otherwise
	 * @return
	 * @throws CDAPException
	 */
	public CDAPMessage getOpenConnectionRequestMessage(int portId, AuthTypes authMech, AuthValue authValue, String destAEInst, String destAEName, String destApInst,
			String destApName, String srcAEInst, String srcAEName, String srcApInst, String srcApName) throws CDAPException{
		CDAPSession cdapSession = this.getCDAPSession(portId);
		if (cdapSession == null){
			cdapSession = this.createCDAPSession(portId);
		}
		return CDAPMessage.getOpenConnectionRequestMessage(authMech, authValue, destAEInst, destAEName, 
				destApInst, destApName, srcAEInst, srcAEName, srcApInst, srcApName, cdapSession.getInvokeIdManager().getInvokeId());
	}
	
	/**
	 * Create a M_CONNECT_R CDAP Message
	 * @param portId identifies the CDAP Session that this message is part of
	 * @param authMech
	 * @param authValue
	 * @param destAEInst
	 * @param destAEName
	 * @param destApInst
	 * @param destApName
	 * @param result
	 * @param resultReason
	 * @param srcAEInst
	 * @param srcAEName
	 * @param srcApInst
	 * @param srcApName
	 * @param invokeId
	 * @return
	 * @throws CDAPException
	 */
	public CDAPMessage getOpenConnectionResponseMessage(int portId, AuthTypes authMech, AuthValue authValue, String destAEInst, String destAEName, String destApInst,
			String destApName, int result, String resultReason, String srcAEInst, String srcAEName, String srcApInst, String srcApName, 
			int invokeId) throws CDAPException{
		return CDAPMessage.getOpenConnectionResponseMessage(authMech, authValue, destAEInst, destAEName, destApInst, destApName, result, 
				resultReason, srcAEInst, srcAEName, srcApInst, srcApName, invokeId);
	}
	
	/**
	 * Create an M_RELEASE CDAP Message
	 * @param portId identifies the CDAP Session that this message is part of
	 * @param flags
	 * @param invokeID
	 * @return
	 * @throws CDAPException
	 */
	public CDAPMessage getReleaseConnectionRequestMessage(int portId, Flags flags, boolean invokeID) throws CDAPException{
		CDAPMessage cdapMessage = CDAPMessage.getReleaseConnectionRequestMessage(flags);
		assignInvokeId(cdapMessage, invokeID, portId);
		
		return cdapMessage;
	}
	
	/**
	 * Create a M_RELEASE_R CDAP Message
	 * @param portId identifies the CDAP Session that this message is part of
	 * @param flags
	 * @param result
	 * @param resultReason
	 * @param invokeId
	 * @return
	 * @throws CDAPException
	 */
	public CDAPMessage getReleaseConnectionResponseMessage(int portId, Flags flags, int result, String resultReason, 
			int invokeId) throws CDAPException{
		return CDAPMessage.getReleaseConnectionResponseMessage(flags, result, resultReason, invokeId);
	}
	
	/**
	 * Create a M_CREATE CDAP Message
	 * @param portId identifies the CDAP Session that this message is part of
	 * @param filter
	 * @param flags
	 * @param objClass
	 * @param objInst
	 * @param objName
	 * @param objValue
	 * @param scope
	 * @param invokeId
	 * @return
	 * @throws CDAPException
	 */
	public CDAPMessage getCreateObjectRequestMessage(int portId, byte[] filter, Flags flags, String objClass, long objInst, String objName, ObjectValue objValue, 
			int scope, boolean invokeId) throws CDAPException{
		CDAPMessage cdapMessage = CDAPMessage.getCreateObjectRequestMessage(filter, flags, objClass, objInst, objName, objValue, scope);
		assignInvokeId(cdapMessage, invokeId, portId);
		return cdapMessage;
	}
	
	/**
	 * Crate a M_CREATE_R CDAP Message
	 * @param portId identifies the CDAP Session that this message is part of
	 * @param flags
	 * @param objClass
	 * @param objInst
	 * @param objName
	 * @param objValue
	 * @param result
	 * @param resultReason
	 * @param invokeId
	 * @return
	 * @throws CDAPException
	 */
	public CDAPMessage getCreateObjectResponseMessage(int portId, Flags flags, String objClass, long objInst, String objName, ObjectValue objValue, int result,
			String resultReason, int invokeId) throws CDAPException{
		return CDAPMessage.getCreateObjectResponseMessage(flags, objClass, objInst, objName, objValue, result, resultReason, invokeId);
	}
	
	/**
	 * Create a M_DELETE CDAP Message
	 * @param portId identifies the CDAP Session that this message is part of
	 * @param filter
	 * @param flags
	 * @param objClass
	 * @param objInst
	 * @param objName
	 * @param objectValue
	 * @param scope
	 * @param invokeId
	 * @return
	 * @throws CDAPException
	 */
	public CDAPMessage getDeleteObjectRequestMessage(int portId, byte[] filter, Flags flags, String objClass, long objInst, String objName, 
			ObjectValue objectValue, int scope, boolean invokeId) throws CDAPException{
		CDAPMessage cdapMessage = CDAPMessage.getDeleteObjectRequestMessage(filter, flags, objClass, objInst, objName, objectValue, scope);
		assignInvokeId(cdapMessage, invokeId, portId);
		
		return cdapMessage;
	}
	
	/**
	 * Create a M_DELETE_R CDAP MESSAGE
	 * @param portId identifies the CDAP Session that this message is part of
	 * @param flags
	 * @param objClass
	 * @param objInst
	 * @param objName
	 * @param result
	 * @param resultReason
	 * @param invokeId
	 * @return
	 * @throws CDAPException
	 */
	public CDAPMessage getDeleteObjectResponseMessage(int portId, Flags flags, String objClass, long objInst, String objName, int result, String resultReason, 
			int invokeId) throws CDAPException{
		return CDAPMessage.getDeleteObjectResponseMessage(flags, objClass, objInst, objName, result, resultReason, invokeId);
	}
	
	/**
	 * Create a M_START CDAP Message
	 * @param portId identifies the CDAP Session that this message is part of
	 * @param filter
	 * @param flags
	 * @param objClass
	 * @param objValue
	 * @param objInst
	 * @param objName
	 * @param scope
	 * @param invokeId
	 * @return
	 * @throws CDAPException
	 */
	public CDAPMessage getStartObjectRequestMessage(int portId, byte[] filter, Flags flags, String objClass, ObjectValue objValue, long objInst, String objName, 
			int scope, boolean invokeId) throws CDAPException{
		CDAPMessage cdapMessage = CDAPMessage.getStartObjectRequestMessage(filter, flags, objClass, objValue, objInst, objName, scope);
		assignInvokeId(cdapMessage, invokeId, portId);
		
		return cdapMessage;
	}
	
	/**
	 * Create a M_START_R CDAP Message
	 * @param portId identifies the CDAP Session that this message is part of
	 * @param flags
	 * @param result
	 * @param resultReason
	 * @param invokeId
	 * @return
	 * @throws CDAPException
	 */
	public CDAPMessage getStartObjectResponseMessage(int portId, Flags flags, int result, String resultReason, int invokeId) throws CDAPException{
		return CDAPMessage.getStartObjectResponseMessage(flags, result, resultReason, invokeId);
	}
	
	/**
	 * Create a M_START_R CDAP Message
	 * @param portId identifies the CDAP Session that this message is part of
	 * @param flags
	 * @param result
	 * @param resultReason
	 * @param invokeId
	 * @return
	 * @throws CDAPException
	 */
	public CDAPMessage getStartObjectResponseMessage(int portId, Flags flags, String objClass, ObjectValue objValue, long objInst, String objName, 
			int result, String resultReason, int invokeId) throws CDAPException{
		return CDAPMessage.getStartObjectResponseMessage(flags, objClass, objValue, objInst, objName, result, resultReason, invokeId);
	}
	
	/**
	 * Create a M_STOP CDAP Message
	 * @param portId identifies the CDAP Session that this message is part of
	 * @param filter
	 * @param flags
	 * @param objClass
	 * @param objValue
	 * @param objInst
	 * @param objName
	 * @param scope
	 * @param invokeId
	 * @return
	 * @throws CDAPException
	 */
	public CDAPMessage getStopObjectRequestMessage(int portId, byte[] filter, Flags flags, String objClass, ObjectValue objValue, long objInst, 
			String objName, int scope, boolean invokeId) throws CDAPException{
		CDAPMessage cdapMessage = CDAPMessage.getStopObjectRequestMessage(filter, flags, objClass, objValue, objInst, objName, scope);
		assignInvokeId(cdapMessage, invokeId, portId);
		
		return cdapMessage;
	}
	
	/**
	 * Create a M_STOP_R CDAP Message
	 * @param portId identifies the CDAP Session that this message is part of
	 * @param flags
	 * @param result
	 * @param resultReason
	 * @param invokeId
	 * @return
	 * @throws CDAPException
	 */
	public CDAPMessage getStopObjectResponseMessage(int portId, Flags flags, int result, String resultReason, int invokeId) throws CDAPException{
		return CDAPMessage.getStopObjectResponseMessage(flags, result, resultReason, invokeId);
	}
	
	/**
	 * Create a M_READ CDAP Message
	 * @param portId identifies the CDAP Session that this message is part of
	 * @param filter
	 * @param flags
	 * @param objClass
	 * @param objInst
	 * @param objName
	 * @param scope
	 * @param invokeId
	 * @return
	 * @throws CDAPException
	 */
	public CDAPMessage getReadObjectRequestMessage(int portId, byte[] filter, Flags flags, String objClass, long objInst, String objName, int scope, 
			boolean invokeId) throws CDAPException{
		CDAPMessage cdapMessage = CDAPMessage.getReadObjectRequestMessage(filter, flags, objClass, objInst, objName, scope);
		assignInvokeId(cdapMessage, invokeId, portId);
		
		return cdapMessage;
	}
	
	/**
	 * Crate a M_READ_R CDAP Message
	 * @param portId identifies the CDAP Session that this message is part of
	 * @param flags
	 * @param objClass
	 * @param objInst
	 * @param objName
	 * @param objValue
	 * @param result
	 * @param resultReason
	 * @param invokeId
	 * @return
	 * @throws CDAPException
	 */
	public CDAPMessage getReadObjectResponseMessage(int portId, Flags flags, String objClass, long objInst, String objName, ObjectValue objValue, 
			int result, String resultReason, int invokeId) throws CDAPException{
		return CDAPMessage.getReadObjectResponseMessage(flags, objClass, objInst, objName, objValue, result, resultReason, invokeId);
	}
	
	/**
	 * Create a M_WRITE CDAP Message
	 * @param portId identifies the CDAP Session that this message is part of
	 * @param filter
	 * @param flags
	 * @param objClass
	 * @param objInst
	 * @param objValue
	 * @param objName
	 * @param scope
	 * @param invokeId
	 * @return
	 * @throws CDAPException
	 */
	public CDAPMessage getWriteObjectRequestMessage(int portId, byte[] filter, Flags flags, String objClass, long objInst, ObjectValue objValue, 
			String objName, int scope, boolean invokeId) throws CDAPException{
		CDAPMessage cdapMessage = CDAPMessage.getWriteObjectRequestMessage(filter, flags, objClass, objInst, objValue, objName, scope);
		assignInvokeId(cdapMessage, invokeId, portId);
		
		return cdapMessage;
	}
	
	/**
	 * Create a M_WRITE_R CDAP Message
	 * @param portId identifies the CDAP Session that this message is part of
	 * @param flags
	 * @param result
	 * @param resultReason
	 * @param invokeId
	 * @return
	 * @throws CDAPException
	 */
	public CDAPMessage getWriteObjectResponseMessage(int portId, Flags flags, int result, String resultReason, int invokeId) throws CDAPException{
		return CDAPMessage.getWriteObjectResponseMessage(flags, result, invokeId, resultReason);
	}
	
	/**
	 * Create a M_CANCELREAD CDAP Message
	 * @param portId identifies the CDAP Session that this message is part of
	 * @param flags
	 * @param invokeID
	 * @return
	 * @throws CDAPException
	 */
	public CDAPMessage getCancelReadRequestMessage(int portId, Flags flags, int invokeID) throws CDAPException{
		return CDAPMessage.getCancelReadRequestMessage(flags, invokeID);
	}
	
	/**
	 * Create a M_CANCELREAD_R CDAP Message
	 * @param portId identifies the CDAP Session that this message is part of
	 * @param flags
	 * @param invokeID
	 * @param result
	 * @param resultReason
	 * @return
	 * @throws CDAPException
	 */
	public CDAPMessage getCancelReadResponseMessage(int portId, Flags flags, int invokeID, int result, String resultReason) throws CDAPException{
		return CDAPMessage.getCancelReadResponseMessage(flags, invokeID, result, resultReason);
	}
	
	private void assignInvokeId(CDAPMessage cdapMessage, boolean invokeId,  int portId){
		if(invokeId){
			CDAPSession cdapSession = this.getCDAPSession(portId);
			cdapMessage.setInvokeID(cdapSession.getInvokeIdManager().getInvokeId());
		}
	}
}