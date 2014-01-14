package rina.ipcprocess.impl.PDUForwardingTable;

import java.util.AbstractQueue;
import java.util.concurrent.LinkedBlockingQueue;

import rina.ipcprocess.impl.PDUForwardingTable.ribobjects.*;

public class FlowStateDatabase {
	/**
	 * The FlowStateObjectGroup
	 */
	private FlowStateObjectGroup flowStateObjectGroup = null;
	
	/**
	 * Queue of the N-1 portid
	 */
	AbstractQueue<Integer> portQueue = null;

	public FlowStateDatabase()
	{
		portQueue = new LinkedBlockingQueue<Integer>();
	}
	
	public boolean addObjectToGroup(FlowStateObject object)
	{
		return this.flowStateObjectGroup.add(object);
	}
	
	public FlowStateObject getByPortId(int portId)
	{
		return this.flowStateObjectGroup.getByPortId(portId);
	}
}
