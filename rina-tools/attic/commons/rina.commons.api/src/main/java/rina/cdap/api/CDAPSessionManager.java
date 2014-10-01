package rina.cdap.api;

import rina.cdap.api.message.AuthValue;
import rina.cdap.api.message.CDAPMessage;
import rina.cdap.api.message.ObjectValue;
import rina.cdap.api.message.CDAPMessage.AuthTypes;
import rina.cdap.api.message.CDAPMessage.Flags;

/**
 * Manages the creation/deletion of CDAP sessions within an IPC process
 * @author eduardgrasa
 *
 */
public interface CDAPSessionManager {
	
	/**
	 * Depending on the message received, it will create a new CDAP state machine (CDAP Session), or update 
	 * an existing one, or terminate one.
	 * @param encodedCDAPMessage
	 * @param portId
	 * @return Decoded CDAP Message
	 * @throws CDAPException if the message is not consistent with the appropriate CDAP state machine
	 */
	public CDAPMessage messageReceived(byte[] encodedCDAPMessage, int portId) throws CDAPException;
	
	/**
	 * Encodes the next CDAP message to be sent, and checks against the 
	 * CDAP state machine that this is a valid message to be sent
	 * @param cdapMessage The cdap message to be serialized
	 * @param portId 
	 * @return encoded version of the CDAP Message
	 * @throws CDAPException
	 */
	public byte[] encodeNextMessageToBeSent(CDAPMessage cdapMessage, int portId) throws CDAPException;
	
	/**
	 * Update the CDAP state machine because we've sent a message through the
	 * flow identified by portId
	 * @param cdapMessage The CDAP message to be serialized
	 * @param portId 
	 * @return encoded version of the CDAP Message
	 * @throws CDAPException
	 */
	public void messageSent(CDAPMessage cdapMessage, int portId) throws CDAPException;
	
	/**
	 * Get a CDAP session that matches the portId
	 * @param portId
	 * @return
	 */
	public CDAPSession getCDAPSession(int portId);
	
	/**
	 * Get the identifiers of all the CDAP sessions
	 * @return
	 */
	public int[] getAllCDAPSessionIds();
	
	/**
	 * Called by the CDAPSession state machine when the cdap session is terminated
	 * @param portId
	 */
	public void removeCDAPSession(int portId);
	
	/**
	 * Encodes a CDAP message. It just converts a CDAP message into a byte 
	 * array, without caring about what session this CDAP message belongs to (and 
	 * therefore it doesn't update any CDAP session state machine). Called by 
	 * functions that have to relay CDAP messages, and need to 
	 * encode its contents to make the relay decision and maybe modify some 
	 * message values.
	 * @param cdapMessage
	 * @return
	 * @throws CDAPException
	 */
	public byte[] encodeCDAPMessage(CDAPMessage cdapMessage) throws CDAPException;
	
	/**
	 * Decodes a CDAP message. It just converts the byte array into a CDAP 
	 * message, without caring about what session this CDAP message belongs to (and 
	 * therefore it doesn't update any CDAP session state machine). Called by 
	 * functions that have to relay CDAP messages, and need to serialize/
	 * decode its contents to make the relay decision and maybe modify some 
	 * @param cdapMessage
	 * @return
	 * @throws CDAPException
	 */
	public CDAPMessage decodeCDAPMessage(byte[] cdapMessage) throws CDAPException;
	
	/**
	 * Return the portId of the (N-1) Flow that supports the CDAP Session
	 * with the IPC process identified by destinationApplicationProcessName and destinationApplicationProcessInstance
	 * @param destinationApplicationProcessName
	 * @throws CDAPException
	 */
	public int getPortId(String destinationApplicationProcessName) throws CDAPException;
	
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
			String destApName, String srcAEInst, String srcAEName, String srcApInst, String srcApName) throws CDAPException;
	
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
			int invokeId) throws CDAPException;
	
	/**
	 * Create an M_RELEASE CDAP Message
	 * @param portId identifies the CDAP Session that this message is part of
	 * @param flags
	 * @param invokeID
	 * @return
	 * @throws CDAPException
	 */
	public CDAPMessage getReleaseConnectionRequestMessage(int portId, Flags flags, boolean invokeID) throws CDAPException;
	
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
			int invokeId) throws CDAPException;
	
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
			int scope, boolean invokeId) throws CDAPException;
	
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
			String resultReason, int invokeId) throws CDAPException;
	
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
			ObjectValue objectValue, int scope, boolean invokeId) throws CDAPException;
	
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
			int invokeId) throws CDAPException;
	
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
			int scope, boolean invokeId) throws CDAPException;
	
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
	public CDAPMessage getStartObjectResponseMessage(int portId, Flags flags, int result, String resultReason, int invokeId) throws CDAPException;
	
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
			int result, String resultReason, int invokeId) throws CDAPException;
	
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
			String objName, int scope, boolean invokeId) throws CDAPException;
	
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
	public CDAPMessage getStopObjectResponseMessage(int portId, Flags flags, int result, String resultReason, int invokeId) throws CDAPException;
	
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
			boolean invokeId) throws CDAPException;
	
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
			int result, String resultReason, int invokeId) throws CDAPException;
	
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
			String objName, int scope, boolean invokeId) throws CDAPException;
	
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
	public CDAPMessage getWriteObjectResponseMessage(int portId, Flags flags, int result, String resultReason, int invokeId) throws CDAPException;
	
	/**
	 * Create a M_CANCELREAD CDAP Message
	 * @param portId identifies the CDAP Session that this message is part of
	 * @param flags
	 * @param invokeID
	 * @return
	 * @throws CDAPException
	 */
	public CDAPMessage getCancelReadRequestMessage(int portId, Flags flags, int invokeID) throws CDAPException;
	
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
	public CDAPMessage getCancelReadResponseMessage(int portId, Flags flags, int invokeID, int result, String resultReason) throws CDAPException;	
}
