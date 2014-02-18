package rina.adataunit.api;

/**
 * 
 * @author eduardgrasa
 *
 */
public class ADataUnitPDU {

	public static final String ADataUnitPDUObjectName = "/daf/adataunitpdu";
	
	/** The address of the Application Process that generated this PDU */
	private long sourceAddresss = 0;
	
	/** The address of the Applicatio Process that is the target this PDU*/
	private long destinationAddress = 0;
	
	/** The encoded payload of the PDU */
	private byte[] payload = null;
	
	public ADataUnitPDU(){
	}
	
	public ADataUnitPDU(long sourceAddress, long destinationAddress, byte[] payload) {
		this.sourceAddresss = sourceAddress;
		this.destinationAddress = destinationAddress;
		this.payload = payload;
	}

	public long getSourceAddresss() {
		return sourceAddresss;
	}

	public void setSourceAddresss(long sourceAddresss) {
		this.sourceAddresss = sourceAddresss;
	}

	public long getDestinationAddress() {
		return destinationAddress;
	}

	public void setDestinationAddress(long destinationAddress) {
		this.destinationAddress = destinationAddress;
	}

	public byte[] getPayload() {
		return payload;
	}

	public void setPayload(byte[] payload) {
		this.payload = payload;
	}
}
