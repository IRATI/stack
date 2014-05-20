package rina.ipcprocess.impl.pduftg.linkstate.ribobjects;

import rina.ipcprocess.api.IPCProcess;
import rina.pduftg.api.linkstate.FlowStateObject;
import rina.ribdaemon.api.SimpleSetMemberRIBObject;

public class FlowStateRIBObject extends SimpleSetMemberRIBObject{
	
	public FlowStateRIBObject(FlowStateObject object, IPCProcess ipcProcess, String objectName) {
		super(ipcProcess, FlowStateObject.FLOW_STATE_RIB_OBJECT_CLASS, objectName, object);
	}
	
}
