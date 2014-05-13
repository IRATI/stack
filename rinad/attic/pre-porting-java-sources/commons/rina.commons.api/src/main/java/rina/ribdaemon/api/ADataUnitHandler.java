package rina.ribdaemon.api;

import rina.cdap.api.CDAPMessageHandler;
import rina.cdap.api.message.CDAPMessage;
import eu.irati.librina.IPCException;
import eu.irati.librina.PDUForwardingTableEntryList;

public interface ADataUnitHandler {

	/** Set the new A-Data PDU Forwarding Table */
	public void setPDUForwardingTable(PDUForwardingTableEntryList entries);
	
	/** Get the port-id of the N-1 flow to reach the destination address*/
	public long getNextHop(long destinationAddress) throws IPCException;
	
	/** Send a message encapsulated in an A-Data-Unit PDU */
	public void sendADataUnit(long destinationAddress, CDAPMessage cdapMessage,
			CDAPMessageHandler cdapMessageHandler) throws IPCException;
	
}
