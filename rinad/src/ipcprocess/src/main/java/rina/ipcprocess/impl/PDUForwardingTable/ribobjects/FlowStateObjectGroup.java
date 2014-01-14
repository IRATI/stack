package rina.ipcprocess.impl.PDUForwardingTable.ribobjects;

import java.util.ArrayList;

public class FlowStateObjectGroup {
	protected ArrayList<FlowStateObject> flowStateObjectArray = null;

	/*		Constructors		*/
	public FlowStateObjectGroup()
	{
		flowStateObjectArray = new ArrayList<FlowStateObject>();
	}
	public FlowStateObjectGroup(FlowStateObject object)
	{
		flowStateObjectArray = new ArrayList<FlowStateObject>();
		flowStateObjectArray.add(object);
	}
	public FlowStateObjectGroup(ArrayList<FlowStateObject> objects)
	{
		flowStateObjectArray = objects;
	}
	
	/**
	 * Add a FlowStateObject to the group
	 * @param object
	 * @return 
	 */
	public boolean add(FlowStateObject object)
	{
		return flowStateObjectArray.add(object);
	}
	
	/**
	 * Get the object given its port
	 * @param portId
	 * @return the flow state object
	 */
	public FlowStateObject getByPortId(int portId)
	{
		int i = 0;
		while (this.flowStateObjectArray.get(i).portid != portId && i < this.flowStateObjectArray.size())
		{
			i++;
		}
		
		if (i == this.flowStateObjectArray.size()-1)
		{
			return null;
		}
		else
		{
			return this.flowStateObjectArray.get(i);
		}
	}
}
