package rina.cdap.api;

import rina.cdap.api.message.CDAPMessage;

/**
 * Represents a CDAP session. Clients of the library are the ones managing the invoke ids. Application entities must 
 * use the CDAP library this way:
 *  1) when sending a message:
 *     a) create the CDAPMessage
 *     b) call serializeNextMessageToBeSent()
 *     c) if it is successful, send the byte[] through the underlying transport connection
 *     d) if successful, update the CDAPSession state machine by calling messageSent()
 *  2) when receiving a message:
 *     a) call the messageReceived operation
 *     b) if successful, you can already use the cdap message; if not, look at the exception
 */
public interface CDAPSession {
	
	/**
	 * Encodes the next CDAP message to be sent, and checks against the 
	 * CDAP state machine that this is a valid message to be sent
	 * @param message
	 * @return the serialized request message, ready to be sent to a flow
	 * @throws CDAPException if the message is bad formed or inconsistent with the protocol state machine
	 */
	public byte[] encodeNextMessageToBeSent(CDAPMessage message) throws CDAPException;
	
	/**
	 * Tell the CDAP state machine that we've just sent the cdap Message, 
	 * so the internal state machine will be updated
	 * @param message
	 * @throws CDAPException if the message is bad formed or inconsistent with the protocol state machine
	 */
	public void messageSent(CDAPMessage message) throws CDAPException;
	
	/**
	 * Tell the CDAP state machine that we've received a message, and get 
	 * the deserialized CDAP message. The state of the CDAP state machine will be updated
	 * @param cdapMessage
	 * @return
	 * @throws CDAPException if the message is bad formed or inconsistent with the protocol state machine
	 */
	public CDAPMessage messageReceived(byte[] cdapMessage) throws CDAPException;
	
	/**
	 * Tell the CDAP state machine that we've received a message. The state of the CDAP state machine will be updated
	 * @param cdapMessage
	 * @return
	 * @throws CDAPException if the message is bad formed or inconsistent with the protocol state machine
	 */
	public CDAPMessage messageReceived(CDAPMessage cdapMessage) throws CDAPException;
	
	/**
	 * Getter for the portId
	 * @return a String that identifies a CDAP session within an IPC process
	 */
	public int getPortId();
	
	/**
	 * Getter for the sessionDescriptor
	 * @return the SessionDescriptor, provides all the data that describes this CDAP session (src and dest naming info, 
	 * authentication type, version, ...)
	 */
	public CDAPSessionDescriptor getSessionDescriptor();
	
	public CDAPInvokeIdManager getInvokeIdManager();
}
