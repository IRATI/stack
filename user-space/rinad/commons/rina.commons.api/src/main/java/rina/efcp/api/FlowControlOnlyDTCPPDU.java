package rina.efcp.api;

/**
 * Represents a Flow Control only DTCP PDU
 * @author eduardgrasa
 *
 */
public class FlowControlOnlyDTCPPDU extends PDU{
	
	/**
	 * This field contains the sequence number of the right window edge. Using this rather than a distinct credit 
	 * field that is an offset from the ACK field allows flow control to be completely independent of 
	 * retransmission control.
	 */
	private long rightWindowEdge = 0;
	
	/**
	 * This field contains the value of the rate that the receiver is giving to the sender. The Sender is allowed to 
	 * send this many PDUs in the given Time Unit.
	 */
	private long newRate = 0;
	
	/**
	 * This field contains the unit of Time in milliseconds over which the rate is computed.
	 */
	private long timeUnit = 0;
	
	public FlowControlOnlyDTCPPDU(){
		this.setPduType(PDU.FLOW_CONTROL_ONLY_DTCP_PDU);
	}
	
	public FlowControlOnlyDTCPPDU(PDU pdu){
		this.setDestinationAddress(pdu.getDestinationAddress());
		this.setSourceAddress(pdu.getSourceAddress());
		this.setConnectionId(pdu.getConnectionId());
		this.setSequenceNumber(pdu.getSequenceNumber());
		this.setPduType(pdu.getPduType());
		this.setEncodedPCI(pdu.getEncodedPCI());
		this.setErrorCheckCode(pdu.getErrorCheckCode());
		this.setRawPDU(pdu.getRawPDU());
		this.setUserData(pdu.getUserData());
		this.setFlags(pdu.getFlags());
	}
	
	public long getRightWindowEdge() {
		return rightWindowEdge;
	}

	public void setRightWindowEdge(long rightWindowEdge) {
		this.rightWindowEdge = rightWindowEdge;
	}

	public long getNewRate() {
		return newRate;
	}

	public void setNewRate(long newRate) {
		this.newRate = newRate;
	}

	public long getTimeUnit() {
		return timeUnit;
	}

	public void setTimeUnit(long timeUnit) {
		this.timeUnit = timeUnit;
	}

	@Override
	public String toString(){
		String result = "";
		result = result + "Destination @: " +this.getDestinationAddress() + " CEPid: " + this.getConnectionId().getDestinationCEPId() + 
			" Source @: " +this.getSourceAddress() + " CEPid: " +this.getConnectionId().getSourceCEPId() + "\n" + 
			" QoSid: " +this.getConnectionId().getQosId() + " PDU type: " +this.getPduType() + 
			" Sequence number: " +this.getSequenceNumber() + " Right window edge: " +this.getRightWindowEdge() + 
			" New rate: "+ this.getNewRate() + " Time Unit: "+ this.getTimeUnit();
		
		return result;
	}
}
