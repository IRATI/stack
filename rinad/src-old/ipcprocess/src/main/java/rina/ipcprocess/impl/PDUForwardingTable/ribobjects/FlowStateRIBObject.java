package rina.ipcprocess.impl.PDUForwardingTable.ribobjects;

import rina.PDUForwardingTable.api.FlowStateObject;
import rina.ipcprocess.api.IPCProcess;
import rina.ribdaemon.api.SimpleSetMemberRIBObject;

public class FlowStateRIBObject extends SimpleSetMemberRIBObject{
	
	public FlowStateRIBObject(FlowStateObject object, IPCProcess ipcProcess, String objectName) {
		super(ipcProcess, FlowStateObject.FLOW_STATE_RIB_OBJECT_CLASS, objectName, object);
	}
	
}
