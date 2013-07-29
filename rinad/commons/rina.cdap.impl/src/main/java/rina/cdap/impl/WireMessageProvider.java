package rina.cdap.impl;

import rina.cdap.api.CDAPException;
import rina.cdap.api.message.CDAPMessage;

/**
 * Provides a wire format for CDAP messages
 * @author eduardgrasa
 *
 */
public interface WireMessageProvider {

	/**
	 * Convert from wire format to CDAPMessage
	 * @param message
	 * @return
	 * @throws CDAPException
	 */
	public CDAPMessage deserializeMessage(byte[] message) throws CDAPException;
	
	/**
	 * Convert from CDAP messages to wire format
	 * @param cdapMessage
	 * @return
	 * @throws CDAPException
	 */
	public byte[] serializeMessage(CDAPMessage cdapMessage) throws CDAPException;
}
