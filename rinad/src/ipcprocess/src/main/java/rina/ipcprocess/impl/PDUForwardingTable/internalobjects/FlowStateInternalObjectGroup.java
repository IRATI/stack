package rina.ipcprocess.impl.PDUForwardingTable.internalobjects;

import java.util.ArrayList;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import rina.PDUForwardingTable.api.FlowStateObject;
import rina.PDUForwardingTable.api.FlowStateObjectGroup;
import rina.ipcprocess.impl.PDUForwardingTable.PDUFTImpl;
import rina.ipcprocess.impl.PDUForwardingTable.ribobjects.FlowStateRIBObjectGroup;
import rina.ribdaemon.api.ObjectInstanceGenerator;
import rina.ribdaemon.api.RIBDaemonException;

public class FlowStateInternalObjectGroup{
	private static final Log log = LogFactory.getLog(PDUFTImpl.class);
	
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
	 * @throws RIBDaemonException 
	 */
	public void add(FlowStateInternalObject internalObject, FlowStateRIBObjectGroup fsRIBGroup ) throws RIBDaemonException
	{
		ObjectStateMapper mapper = new ObjectStateMapper();
		
		flowStateObjectArray.add(internalObject);
		FlowStateObject object = mapper.FSOMap(internalObject);
		fsRIBGroup.create(FlowStateObject.FLOW_STATE_RIB_OBJECT_CLASS, ObjectInstanceGenerator.getObjectInstance(), object.getID(), object);
	}
	
	/**
	 * Add a FlowStateObject to a group for sending
	 * @param object
	 * @return 
	 * @throws RIBDaemonException 
	 */
	public void addToSend(FlowStateInternalObject internalObject)
	{
		flowStateObjectArray.add(internalObject);
	}
	
	/**
	 * Get the object given its port
	 * @param portId
	 * @return the flow state object
	 */
	public FlowStateInternalObject getByPortId(int portId)
	{
		int i = 0;
		while (i < this.flowStateObjectArray.size() && this.flowStateObjectArray.get(i).portid != portId)
		{
			i++;
		}
		
		if (i == this.flowStateObjectArray.size())
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
			return modifiedFSOs;
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
				log.debug("Old object removed: " + object);
				this.flowStateObjectArray.remove(i);
				groupModified = true;
			}
		}
		return groupModified;
	}
}
