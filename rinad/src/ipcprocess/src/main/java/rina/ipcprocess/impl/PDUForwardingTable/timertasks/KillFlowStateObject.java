package rina.ipcprocess.impl.PDUForwardingTable.timertasks;

import java.util.ArrayList;
import java.util.TimerTask;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import rina.ipcprocess.impl.PDUForwardingTable.FlowStateDatabase;
import rina.ipcprocess.impl.PDUForwardingTable.internalobjects.FlowStateInternalObject;
import rina.ipcprocess.impl.PDUForwardingTable.internalobjects.ObjectStateMapper;
import rina.ipcprocess.impl.PDUForwardingTable.ribobjects.FlowStateRIBObjectGroup;
import rina.ribdaemon.api.RIBDaemonException;

public class KillFlowStateObject extends TimerTask{
	private static final Log log = LogFactory.getLog(KillFlowStateObject.class);
	
	ArrayList<FlowStateInternalObject> flowObjectList = null;
	FlowStateInternalObject objectToRemove = null;
	FlowStateRIBObjectGroup fsRIBGroup = null;
	FlowStateDatabase database = null;
	
	public KillFlowStateObject(FlowStateRIBObjectGroup fsRIB, FlowStateInternalObject object, FlowStateDatabase db)
	{
		flowObjectList = db.getFlowStateInternalObjectGroup().getFlowStateObjectArray();
		objectToRemove = object;
		fsRIBGroup = fsRIB;
		database = db;
	}

	@Override
	public void run() 
	{
		ObjectStateMapper mapper = new ObjectStateMapper();
		
		flowObjectList.remove(objectToRemove);
		try {
			fsRIBGroup.delete(mapper.FSOMap(objectToRemove));
			database.setModified(true);
			log.debug("Old object removed: " + objectToRemove.getID());
		} catch (RIBDaemonException e) {
			log.error("Object could not be removed from the RIB");
			e.printStackTrace();
		}
	}
}
