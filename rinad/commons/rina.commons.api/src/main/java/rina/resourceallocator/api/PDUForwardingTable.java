package rina.resourceallocator.api;

/**
 * Classes implementing this interface contain information of what N-1 flow 
 * has to be used to send an N-PDU, based on the values of the fields of the PDU's PCI.
 * @author eduardgrasa
 *
 */
public interface PDUForwardingTable {
	
	/**
	 * Returns the N-1 portIds through which the N PDU has to be sent
	 * @param destinationAddress
	 * @param qosId
	 * @return
	 */
	public int[] getNMinusOnePortId(long destinationAddress, int qosId);
	
	/**
	 * Add an entry to the forwarding table
	 * @param destinationAddress
	 * @param qosId
	 * @param portId the portId associated to the destination_address-qosId-destination_CEP_id 
	 */
	public void addEntry(long destinationAddress, int qosId, int[] portIds);
	
	/**
	 * Remove an entry from the forwarding table
	 * @param destinationAddress
	 * @param qosId
	 */
	public void removeEntry(long destinationAddress, int qosId);
}
