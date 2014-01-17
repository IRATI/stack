package rina.ipcprocess.impl.PDUForwardingTable.routingalgorithms;

import java.util.List;

import eu.irati.librina.PDUForwardingTableEntryList;
import rina.PDUForwardingTable.api.FlowStateObject;


public interface RoutingAlgorithmInt {
	
	/**
	 * Compute the path to other nodes and updates the PDU forwarding Table
	 * @return
	 */
	public PDUForwardingTableEntryList getPDUTForwardingTable(List<FlowStateObject> fsoList, AbstractVertex source);

}
