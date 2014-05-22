package rina.ipcprocess.impl.pduftg.linkstate.timertasks;

import java.util.Timer;
import java.util.TimerTask;

import rina.cdap.api.CDAPSessionManager;
import rina.cdap.api.message.CDAPMessage;
import rina.ipcprocess.impl.pduftg.linkstate.PDUFTCDAPMessageHandler;
import rina.pduftg.api.linkstate.FlowStateObjectGroup;
import rina.ribdaemon.api.RIBDaemon;

public class SendReadCDAP extends TimerTask{
	
	protected int waitUntilError;
	protected int portId;
	protected CDAPSessionManager cdapSessionManager = null;
	protected RIBDaemon ribDaemon = null;
	protected PDUFTCDAPMessageHandler pduftCDAPmessageHandler = null;
	
	public SendReadCDAP(int portId, CDAPSessionManager cdapSessionManager, RIBDaemon ribDaemon,
			int waitUntilError, PDUFTCDAPMessageHandler pduftCDAPmessageHandler)
	{
		this.portId = portId;
		this.waitUntilError = waitUntilError;
		this.cdapSessionManager = cdapSessionManager;
		this.ribDaemon = ribDaemon;
		this.pduftCDAPmessageHandler = pduftCDAPmessageHandler;
	}
	@Override
	public void run() 
	{
		CDAPMessage cdapMessage;
		try {
			cdapMessage = cdapSessionManager.getReadObjectRequestMessage(portId, null, null,
					FlowStateObjectGroup.FLOW_STATE_GROUP_RIB_OBJECT_CLASS, 0, FlowStateObjectGroup.FLOW_STATE_GROUP_RIB_OBJECT_NAME,
					0, false);
			this.ribDaemon.sendMessage(cdapMessage, portId , pduftCDAPmessageHandler);
		} catch (Exception e) {
			e.printStackTrace();
		}

		Timer timer = new Timer();
		timer.schedule(new ErrorCDAP(), waitUntilError);
	}
	
}
