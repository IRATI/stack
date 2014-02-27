package rina.ipcprocess.impl.PDUForwardingTable.timertasks;

import java.util.Timer;
import java.util.TimerTask;

import rina.PDUForwardingTable.api.FlowStateObjectGroup;
import rina.cdap.api.CDAPSessionManager;
import rina.cdap.api.message.CDAPMessage;
import rina.ribdaemon.api.RIBDaemon;


public class SendReadCDAP extends TimerTask{
	
	protected int waitUntilError;
	protected int portId;
	protected CDAPSessionManager cdapSessionManager = null;
	protected RIBDaemon ribDaemon = null;
	
	public SendReadCDAP(int portId, CDAPSessionManager cdapSessionManager, 
			RIBDaemon ribDaemon, int waitUntilError)
	{
		this.portId = portId;
		this.waitUntilError = waitUntilError;
		this.cdapSessionManager = cdapSessionManager;
		this.ribDaemon = ribDaemon;
	}
	@Override
	public void run() {
		
		CDAPMessage cdapMessage;
		try {
			cdapMessage = cdapSessionManager.getReadObjectRequestMessage(portId, null, null,
					FlowStateObjectGroup.FLOW_STATE_GROUP_RIB_OBJECT_CLASS, 0, FlowStateObjectGroup.FLOW_STATE_GROUP_RIB_OBJECT_NAME, 0, false);
			this.ribDaemon.sendMessage(cdapMessage, portId , null);
		} catch (Exception e) {
			e.printStackTrace();
		}

		Timer timer = new Timer();
		timer.schedule(new ErrorCDAP(), waitUntilError);
	}
	
}
