package rina.efcp.api;

/**
 * Represents a Flow Control only DTCP PDU
 * @author eduardgrasa
 *
 */
public class AckOnlyDTCPPDU extends PDU{
	
	/**
	 * If Retransmission Control is operating, this field contains the base 
	 * SequenceNumber being Acked. The sender is to assume that no PDUs with 
	 * SequenceNumbers less than or equal to this one will be requested for 
	 * retransmission. If Retransmission Control is not operating, then policy 
	 * may compute credit relative to this sequence number.
	 */
	private long ack = 0L;
	
	public AckOnlyDTCPPDU(){
		this.setPduType(PDU.ACK_ONLY_DTCP_PDU);
	}
	
	public AckOnlyDTCPPDU(PDU pdu){
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

	public long getAck() {
		return ack;
	}

	public void setAck(long ack) {
		this.ack = ack;
	}

	@Override
	public String toString(){
		String result = "";
		result = result + "Destination @: " +this.getDestinationAddress() + " CEPid: " + this.getConnectionId().getDestinationCEPId() + 
			" Source @: " +this.getSourceAddress() + " CEPid: " +this.getConnectionId().getSourceCEPId() + "\n" + 
			" QoSid: " +this.getConnectionId().getQosId() + " PDU type: " +this.getPduType() + 
			" Sequence number: " +this.getSequenceNumber() + " Ack: " +this.getAck();
		
		return result;
	}
}
