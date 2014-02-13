package rina.ipcprocess.impl.PDUForwardingTable.ribobjects;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import rina.PDUForwardingTable.api.FlowStateObject;
import rina.ipcprocess.api.IPCProcess;
import rina.ribdaemon.api.SimpleSetMemberRIBObject;

public class FlowStateRIBObject extends SimpleSetMemberRIBObject{
	private static final Log log = LogFactory.getLog(FlowStateRIBObject.class);
	
	public FlowStateRIBObject(FlowStateObject object, IPCProcess ipcProcess, String objectName) {
		super(ipcProcess, FlowStateObject.FLOW_STATE_RIB_OBJECT_CLASS, objectName, object);
		log.debug("Created object with id: " + object.getID());
	}
	
}
