package rina.PDUForwardingTable.api;

import java.util.ArrayList;

public class FlowStateObjectGroup {
	protected ArrayList<FlowStateObject> flowStateObjectArray = null;

	/*		Accessors		*/
	public ArrayList<FlowStateObject> getFlowStateObjectArray() {
		return flowStateObjectArray;
	}
	public void setFlowStateObjectArray(
			ArrayList<FlowStateObject> flowStateObjectArray) {
		this.flowStateObjectArray = flowStateObjectArray;
	}
	
	/*		Constructors		*/
	public FlowStateObjectGroup()
	{
		flowStateObjectArray = new ArrayList<FlowStateObject>();
	}
	public FlowStateObjectGroup(ArrayList<FlowStateObject> objects) {
		flowStateObjectArray = objects;
	}
}
