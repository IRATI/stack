package rina.efcp.api;

import rina.ribdaemon.api.RIBObjectNames;

/**
 * Contains the constants for DTP and DTCP, defined at DIF compile time
 * TODO move to a config file after the demo
 * @author eduardgrasa
 *
 */
public class DataTransferConstants {
	
	public static final String DATA_TRANSFER_CONSTANTS_RIB_OBJECT_NAME = RIBObjectNames.SEPARATOR + RIBObjectNames.DIF + 
		RIBObjectNames.SEPARATOR + RIBObjectNames.IPC + RIBObjectNames.SEPARATOR + RIBObjectNames.DATA_TRANSFER+ 
		RIBObjectNames.SEPARATOR + RIBObjectNames.CONSTANTS;

	public static final String DATA_TRANSFER_CONSTANTS_RIB_OBJECT_CLASS = "datatransfercons";
	
	/**
	 * The length of QoS-id field in the DTP PCI, in bytes
	 */
	private int qosIdLength = 0;
	
	/**
	 * The length of the Port-id field in the DTP PCI, in bytes
	 */
	private int portIdLength = 0;
	
	/**
	 * The length of the CEP-id field in the DTP PCI, in bytes
	 */
	private int cepIdLength = 0;
	
	/**
	 * The length of the sequence number field in the DTP PCI, in bytes
	 */
	private int sequenceNumberLength = 0;
	
	/**
	 * The length of the address field in the DTP PCI, in bytes
	 */
	private int addressLength = 0;
	
	/**
	 * The length of the length field in the DTP PCI, in bytes
	 */
	private int lengthLength = 0;
	
	/**
	 * The maximum length allowed for a PDU in this DIF, in bytes
	 */
	private int maxPDUSize = 0;
	
	/**
	 * True if the PDUs in this DIF have CRC, TTL, and/or encryption. Since 
	 * headers are encrypted, not just user data, if any flow uses encryption, 
	 * all flows within the same DIF must do so and the same encryption 
	 * algorithm must be used for every PDU; we cannot identify which flow owns 
	 * a particular PDU until it has been decrypted.
	 */
	private boolean DIFIntegrity = false;
	
	/**
	 * This is true if multiple SDUs can be delimited and concatenated within 
	 * a single PDU.
	 */
	private boolean DIFConcatenation = false;
	
	/**
	 * This is true if multiple SDUs can be fragmented and reassembled within 
	 * a single PDU.
	 */
	private boolean DIFFragmentation = false;
	
	/**
	 * Value of a flag that indicates a SDU fragment that is neither the first nor the last
	 */
	private byte fragmentFlag = 0x00;
	
	/**
	 * Value of a flag that indicates a SDU fragment that is the first fragment of an SDU
	 */
	private byte firstFragmentFlag = 0x01;
	
	/**
	 * Value of a flag that indicates a SDU fragment that is the last fragment of an SDU
	 */
	private byte lastFragmentFlag = 0x02;
	
	/**
	 * Value of a flag that indicates a PDU carrying a complete SDU
	 */
	private byte completeFlag = 0x03;
	
	/**
	 * Value of a flag that indicates a PDU carrying multiple SDUs
	 */
	private byte multipleFlag = 0x07;
	
	/**
	 * The SDU gap timer delay in ms.
	 */
	private long SDUGapTimerDelay = 2*1000;
	
	/**
	 * The maximum PDU lifetime in this DIF, in milliseconds. This is MPL in delta-T
	 */
	private int maxPDULifetime  = 10*1000;
	
	/**
	 * The maximum time DTCP will try to keep retransmitting a PDU, before discarding it. 
	 * This is R in delta-T
	 */
	private int maxTimeToKeepRetransmitting = 10*1000;
    
	/**
	 * /The maximum time the receiving side of a DTCP connection will take to ACK a PDU 
	 * once it has received it. This is A in delta-T
	 */
	private int maxTimeToACK = 10*1000;  

	public int getQosIdLength() {
		return qosIdLength;
	}

	public void setQosIdLength(int qosIdLength) {
		this.qosIdLength = qosIdLength;
	}

	public int getPortIdLength() {
		return portIdLength;
	}

	public void setPortIdLength(int portIdLength) {
		this.portIdLength = portIdLength;
	}

	public int getCepIdLength() {
		return cepIdLength;
	}

	public void setCepIdLength(int cepIdLength) {
		this.cepIdLength = cepIdLength;
	}

	public int getSequenceNumberLength() {
		return sequenceNumberLength;
	}

	public void setSequenceNumberLength(int sequenceNumberLength) {
		this.sequenceNumberLength = sequenceNumberLength;
	}

	public int getAddressLength() {
		return addressLength;
	}

	public void setAddressLength(int addressLength) {
		this.addressLength = addressLength;
	}

	public int getLengthLength() {
		return lengthLength;
	}

	public void setLengthLength(int lengthLength) {
		this.lengthLength = lengthLength;
	}

	public int getPciLength() {
		return 1 + 2*addressLength + qosIdLength + 2*portIdLength + 2 + lengthLength + sequenceNumberLength;
	}

	public int getMaxPDUSize() {
		return maxPDUSize;
	}

	public void setMaxPDUSize(int maxPDUSize) {
		this.maxPDUSize = maxPDUSize;
	}

	public int getMaxSDUSize() {
		return maxPDUSize - getPciLength();
	}

	public boolean isDIFIntegrity() {
		return DIFIntegrity;
	}

	public void setDIFIntegrity(boolean dIFIntegrity) {
		DIFIntegrity = dIFIntegrity;
	}

	public boolean isDIFConcatenation() {
		return DIFConcatenation;
	}

	public void setDIFConcatenation(boolean dIFConcatenation) {
		DIFConcatenation = dIFConcatenation;
	}

	public boolean isDIFFragmentation() {
		return DIFFragmentation;
	}

	public void setDIFFragmentation(boolean dIFFragmentation) {
		DIFFragmentation = dIFFragmentation;
	}

	public byte getFragmentFlag() {
		return fragmentFlag;
	}

	public void setFragmentFlag(byte fragmentFlag) {
		this.fragmentFlag = fragmentFlag;
	}

	public byte getFirstFragmentFlag() {
		return firstFragmentFlag;
	}

	public void setFirstFragmentFlag(byte firstFragmentFlag) {
		this.firstFragmentFlag = firstFragmentFlag;
	}

	public byte getLastFragmentFlag() {
		return lastFragmentFlag;
	}

	public void setLastFragmentFlag(byte lastFragmentFlag) {
		this.lastFragmentFlag = lastFragmentFlag;
	}

	public byte getCompleteFlag() {
		return completeFlag;
	}

	public void setCompleteFlag(byte completeFlag) {
		this.completeFlag = completeFlag;
	}

	public byte getMultipleFlag() {
		return multipleFlag;
	}

	public void setMultipleFlag(byte multipleFlag) {
		this.multipleFlag = multipleFlag;
	}

	public long getSDUGapTimerDelay() {
		return SDUGapTimerDelay;
	}

	public void setSDUGapTimerDelay(long sDUGapTimerDelay) {
		SDUGapTimerDelay = sDUGapTimerDelay;
	}

	public int getMaxPDULifetime() {
		return maxPDULifetime;
	}

	public void setMaxPDULifetime(int maxPDULifetime) {
		this.maxPDULifetime = maxPDULifetime;
	}
	
	public int getMaxTimeToKeepRetransmitting() {
		return maxTimeToKeepRetransmitting;
	}

	public void setMaxTimeToKeepRetransmitting(int maxTimeToKeepRetransmitting) {
		this.maxTimeToKeepRetransmitting = maxTimeToKeepRetransmitting;
	}

	public int getMaxTimeToACK() {
		return maxTimeToACK;
	}

	public void setMaxTimeToACK(int maxTimeToACK) {
		this.maxTimeToACK = maxTimeToACK;
	}

	@Override
	public String toString(){
		String result = "";
		result = result + "Max PDU size: " + this.getMaxPDUSize() + "\n";
		result = result + "Address length: " + this.getAddressLength() + "\n";
		result = result + "Port id length: " + this.getPortIdLength()+ "\n";
		result = result + "Connection endpoint id length: " + this.getCepIdLength() + "\n";
		result = result + "QoS id length: " + this.getQosIdLength() + "\n";
		result = result + "Sequence number length: " + this.getSequenceNumberLength() + "\n";
		result = result + "Packet length length: " + this.getLengthLength() + "\n";
		result = result + "Max PDU Lifetime (ms): " + this.getMaxPDULifetime() + "\n";
		result = result + "Max time to keep retransmitting (ms): " + this.getMaxTimeToKeepRetransmitting() + "\n";
		result = result + "Max time to ACK (ms): " + this.getMaxTimeToACK() + "\n";
		result = result + "DIF concatenation: " + this.isDIFConcatenation() + "\n";
		result = result + "DIF fragmentation: " + this.isDIFFragmentation() + "\n";
		result = result + "DIF integrity: " + this.isDIFIntegrity() + "\n";
		return result;
	}
	
	public static DataTransferConstants getDefaultInstance(){
		DataTransferConstants dataTransferConstants = new DataTransferConstants();
		dataTransferConstants.setAddressLength(2);
		dataTransferConstants.setCepIdLength(2);
		dataTransferConstants.setDIFConcatenation(true);
		dataTransferConstants.setDIFFragmentation(false);
		dataTransferConstants.setDIFIntegrity(false);
		dataTransferConstants.setLengthLength(2);
		dataTransferConstants.setMaxPDUSize(1950);
		dataTransferConstants.setPortIdLength(2);
		dataTransferConstants.setQosIdLength(1);
		dataTransferConstants.setSequenceNumberLength(2);
		
		return dataTransferConstants;
	}
}
