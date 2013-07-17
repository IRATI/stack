package rina.efcp.api;

import rina.flowallocator.api.ConnectionId;

/**
 * Generates/parses EFCP PDUs according to a specific policy
 * @author eduardgrasa
 *
 */
public interface PDUParser {

	/**
	 * Encode the fields that are always the same during the flow lifetime using Little Endian byte order
	 * @param destinationAddress
	 * @param sourceAddress
	 * @param sourceCEPid
	 * @param destinationCEPid
	 * @param qosid
	 * @return
	 */
	public byte[] preComputePCI(long destinationAddress, long sourceAddress, long sourceCEPid, long destinationCEPid, int qosid);
	
	/**
	 * Generate a DTP PDU data structure from a pre-computed DTP PCI. Use unsigned types and little-endian byte order
	 * @param pci
	 * @param sequenceNumber
	 * @param flags
	 * @param userData
	 * @return
	 */
	public PDU generateDTPPDU(byte[] pci, long sequenceNumber, long destinationAddress, 
			ConnectionId connectionId, int flags, byte[] userData);
	
	/**
	 * Generate a DTCP Flow control only PDU
	 * @param pci
	 * @param sequenceNumber
	 * @param destinationAddress
	 * @param connectionId
	 * @param rightWindowEdge
	 * @param newRate
	 * @param timeUnit
	 * @return
	 */
	public FlowControlOnlyDTCPPDU generateFlowControlOnlyDTCPPDU(byte[] pci, long sequenceNumber, long destinationAddress, 
			ConnectionId connectionId, long rightWindowEdge, long newRate, long timeUnit);
	
	/**
	 * Generate a DTCP ACK only PDU
	 * @param pci
	 * @param sequenceNumber
	 * @param destinationAddress
	 * @param connectionId
	 * @param ack
	 * @return
	 */
	public AckOnlyDTCPPDU generateAckOnlyDTCPPDU(byte[] pci, long sequenceNumber, long destinationAddress, 
			ConnectionId connectionId, long ack);
	
	/**
	 * Generate a DTCP Flow control only PDU
	 * @param pci
	 * @param sequenceNumber
	 * @param destinationAddress
	 * @param connectionId
	 * @param rightWindowEdge
	 * @param newRate
	 * @param timeUnit
	 * @return
	 */
	public AckAndFlowControlDTCPPDU generateAckAndFlowControlDTCPPDU(byte[] pci, long sequenceNumber, long destinationAddress, 
			ConnectionId connectionId, long ack, long rightWindowEdge, long newRate, long timeUnit);
	
	/**
	 * Generates an EFCP Management PDU
	 * @param managementData
	 * @return
	 */
	public PDU generateManagementPDU(byte[] managementData);
	
	public PDU generateIdentifySenderPDU(long address, int qosId);
	
	/**
	 * Parse the values of the encoded PDU relevant to the RMT into a canonical data structure.
	 * Uses unsigned types and little-endian byte order
	 * @param encodedPDU
	 * @return
	 */
	public PDU parsePCIForRMT(PDU pdu);
	
	public PDU parsePCIForEFCP(PDU pdu);
}
