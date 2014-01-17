package rina.ipcprocess.impl.PDUForwardingTable.internalobjects;

import java.util.ArrayList;

import rina.PDUForwardingTable.api.FlowStateObjectGroup;

public class FlowStateInternalObjectGroup{

	protected ArrayList<FlowStateInternalObject> flowStateObjectArray = null;
	
	/*		Acessors		*/
	public ArrayList<FlowStateInternalObject> getFlowStateObjectArray() {
		return flowStateObjectArray;
	}
	public void setFlowStateObjectArray(
			ArrayList<FlowStateInternalObject> flowStateObjectArray) {
		this.flowStateObjectArray = flowStateObjectArray;
	}

	/*		Constructors		*/
	public FlowStateInternalObjectGroup()
	{
		flowStateObjectArray = new ArrayList<FlowStateInternalObject>();
	}
	public FlowStateInternalObjectGroup(ArrayList<FlowStateInternalObject> objects)
	{
		flowStateObjectArray = objects;
	}
	
	
	/**
	 * Add a FlowStateObject to the group
	 * @param object
	 * @return 
	 */
	public boolean add(FlowStateInternalObject object)
	{
		return flowStateObjectArray.add(object);
	}
	
	/**
	 * Get the object given its port
	 * @param portId
	 * @return the flow state object
	 */
	public FlowStateInternalObject getByPortId(int portId)
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
	
	public ArrayList<FlowStateInternalObject> getModifiedFSO()
	{
		ArrayList<FlowStateInternalObject> modifiedFSOs = new ArrayList<FlowStateInternalObject>();
		
		for(int i = 0; i< this.flowStateObjectArray.size(); i++)
		{
			if (this.flowStateObjectArray.get(i).isModified())
			{
				modifiedFSOs.add(this.flowStateObjectArray.get(i));
			}
		}
		
		if (modifiedFSOs.isEmpty())
		{
			return null;
		}
		else
		{
			return modifiedFSOs;
		}
	}
	
	public boolean incrementAge(int maximumAge)
	{
		boolean groupModified = false;
		
		for(int i = 0; i< this.flowStateObjectArray.size(); i++)
		{
			FlowStateInternalObject object = this.flowStateObjectArray.get(i);
			object.incrementAge();
			
			if (object.getAge() >= maximumAge)
			{
				this.flowStateObjectArray.remove(i);
				groupModified = true;
			}
		}
		return groupModified;
	}
}
