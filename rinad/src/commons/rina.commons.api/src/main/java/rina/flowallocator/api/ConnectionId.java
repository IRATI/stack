package rina.flowallocator.api;

/**
 * Encapsulates the data that identifies a connection
 * @author eduardgrasa
 *
 */
public class ConnectionId {
	/**
	 * A DIF-assigned identifier only known within the DIF that stands for a 
	 * particular QoS hypercube.
	 */
	private int qosId = -1;
	
	/**
	 * An identifier unique within the DT-AEI of the source IPC Process that identifies 
	 * the source endpoint of this connection
	 */
	private long sourceCEPId = -1L;
	
	/**
	 * An identifier unique within the DT-AEI of the destination IPC Process that identifies 
	 * the destination endpoint of this connection
	 */
	private long destinationCEPId = -1L;

	public int getQosId() {
		return qosId;
	}

	public void setQosId(int qosId) {
		this.qosId = qosId;
	}

	public long getSourceCEPId() {
		return sourceCEPId;
	}

	public void setSourceCEPId(long sourceCEPId) {
		this.sourceCEPId = sourceCEPId;
	}

	public long getDestinationCEPId() {
		return destinationCEPId;
	}

	public void setDestinationCEPId(long destinationCEPId) {
		this.destinationCEPId = destinationCEPId;
	}
	
	@Override
	public int hashCode() {
		final int prime = 31;
		int result = 1;
		result = prime * result
				+ (int) (destinationCEPId ^ (destinationCEPId >>> 32));
		result = prime * result + qosId;
		result = prime * result + (int) (sourceCEPId ^ (sourceCEPId >>> 32));
		return result;
	}

	@Override
	public boolean equals(Object obj) {
		if (this == obj)
			return true;
		if (obj == null)
			return false;
		if (getClass() != obj.getClass())
			return false;
		ConnectionId other = (ConnectionId) obj;
		if (destinationCEPId != other.destinationCEPId)
			return false;
		if (qosId != other.qosId)
			return false;
		if (sourceCEPId != other.sourceCEPId)
			return false;
		return true;
	}
	
	@Override
	public String toString(){
		String result = "";
		result = result + "QoS id: "+this.qosId + "\n";
		result = result + "Source connection endpoint id: " + this.sourceCEPId + "\n";
		result = result + "Destination connection endpoint id: " + this.destinationCEPId + "\n";
		return result;
	}
}