package rina.pduftg.api.linkstate;

import java.util.List;


import eu.irati.librina.PDUForwardingTableEntryList;


public interface RoutingAlgorithmInt {
	
	/**
	 * Compute the path to other nodes and updates the PDU forwarding Table
	 * @return
	 */
	public PDUForwardingTableEntryList getPDUTForwardingTable(List<FlowStateObject> fsoList, VertexInt source);

}
