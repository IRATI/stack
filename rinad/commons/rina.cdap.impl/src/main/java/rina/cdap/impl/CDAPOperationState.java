package rina.cdap.impl;

import rina.cdap.api.message.CDAPMessage.Opcode;

/**
 * Encapsulates an operation state
 * @author eduardgrasa
 *
 */
public class CDAPOperationState {
	
	/**
	 * The opcode of the message
	 */
	private Opcode opcode = null;
	
	/**
	 * Tells if sender or receiver
	 */
	private boolean sender = true;
	
	public CDAPOperationState(Opcode opcode, boolean sender){
		this.opcode = opcode;
		this.sender = sender;
	}

	public Opcode getOpcode() {
		return opcode;
	}

	public void setOpcode(Opcode opcode) {
		this.opcode = opcode;
	}

	public boolean isSender() {
		return sender;
	}

	public void setSender(boolean sender) {
		this.sender = sender;
	}
	
	

}
