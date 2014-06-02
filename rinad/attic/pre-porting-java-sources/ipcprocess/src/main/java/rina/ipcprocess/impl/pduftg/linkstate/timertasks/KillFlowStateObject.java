package rina.ipcprocess.impl.pduftg.linkstate.timertasks;

import java.util.List;
import java.util.TimerTask;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import rina.ipcprocess.impl.pduftg.linkstate.FlowStateDatabase;
import rina.ipcprocess.impl.pduftg.linkstate.ribobjects.FlowStateRIBObjectGroup;
import rina.pduftg.api.linkstate.FlowStateObject;
import rina.ribdaemon.api.RIBDaemonException;

public class KillFlowStateObject extends TimerTask{
	private static final Log log = LogFactory.getLog(KillFlowStateObject.class);
	
	List<FlowStateObject> flowObjectList = null;
	FlowStateObject objectToRemove = null;
	FlowStateRIBObjectGroup fsRIBGroup = null;
	FlowStateDatabase database = null;
	
	public KillFlowStateObject(FlowStateRIBObjectGroup fsRIB, FlowStateObject object, FlowStateDatabase db)
	{
		flowObjectList = db.getFlowStateObjectGroup().getFlowStateObjectArray();
		objectToRemove = object;
		fsRIBGroup = fsRIB;
		database = db;
	}

	@Override
	public void run() 
	{	
		flowObjectList.remove(objectToRemove);
		try {
			fsRIBGroup.delete(objectToRemove);
			database.setModified(true);
			log.debug("Old object removed: " + objectToRemove.getID());
		} catch (RIBDaemonException e) {
			log.error("Object could not be removed from the RIB");
			e.printStackTrace();
		}
	}
}
