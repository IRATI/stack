package rina.PDUForwardingTable.api;

import java.util.ArrayList;
import java.util.List;

import rina.ribdaemon.api.RIBObjectNames;

public class FlowStateObjectGroup {

	public static final String FLOW_STATE_GROUP_RIB_OBJECT_NAME = RIBObjectNames.SEPARATOR + 
			RIBObjectNames.DIF + RIBObjectNames.SEPARATOR + RIBObjectNames.MANAGEMENT
			+ RIBObjectNames.SEPARATOR + RIBObjectNames.ROUTING + RIBObjectNames.SEPARATOR 
			+ RIBObjectNames.FLOWSTATEOBJECTGROUP;
	
	public static final String FLOW_STATE_GROUP_RIB_OBJECT_CLASS = "flowstateobjectset";
	
	protected List<FlowStateObject> flowStateObjectArray = null;
	
	/*		Accessors		*/
	public List<FlowStateObject> getFlowStateObjectArray() {
		return flowStateObjectArray;
	}
	public void setFlowStateObjectArray(
			List<FlowStateObject> flowStateObjectArray) {
		this.flowStateObjectArray = flowStateObjectArray;
	}
	
	/*		Constructors		*/
	public FlowStateObjectGroup()
	{
		flowStateObjectArray = new ArrayList<FlowStateObject>();
	}
	public FlowStateObjectGroup(List<FlowStateObject> objects) {
		flowStateObjectArray = objects;
	}
}
