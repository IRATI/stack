package rina.ribdaemon.api;

import rina.cdap.api.CDAPSessionDescriptor;
import rina.cdap.api.message.CDAPMessage;

/**
 * 
 * @author eduardgrasa
 */
public interface MessageSubscriber {
	
	/**
	 * Called when a cdapMessage related to an object class we've subscribed to
	 * is received by the RIB Daemon
	 * @param cdapMessage the received CDAP Message
	 * @param cdapSessionDescriptor the descriptor of the session where the CDAP message belongs
	 */
	public void messageReceived(CDAPMessage cdapMessage, CDAPSessionDescriptor cdapSessionDescriptor);
}
