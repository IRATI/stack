package rina.ipcprocess.impl.PDUForwardingTable.timertasks;

import java.util.Timer;
import java.util.TimerTask;

import rina.PDUForwardingTable.api.FlowStateObject;
import rina.cdap.api.CDAPSessionManager;
import rina.cdap.api.message.CDAPMessage;
import rina.ribdaemon.api.RIBDaemon;
import rina.ribdaemon.api.RIBObjectNames;

import eu.irati.librina.ApplicationProcessNamingInformation;

public class SendReadCDAP extends TimerTask{
	
	protected int waitUntilError;
	protected ApplicationProcessNamingInformation name;
	protected long address;
	protected int portId;
	protected CDAPSessionManager cdapSessionManager = null;
	protected RIBDaemon ribDaemon = null;
	
	public SendReadCDAP(ApplicationProcessNamingInformation name, long address, int portId, CDAPSessionManager cdapSessionManager, 
			RIBDaemon ribDaemon, int waitUntilError)
	{
		this.name = name;
		this.address = address;
		this.portId = portId;
		this.waitUntilError = waitUntilError;
	}
	@Override
	public void run() {
		/*
		
		String stateName = ""+ obj.getAddress() +"-"+ obj.getPortid();
		String objName = FlowStateObject.FLOW_STATE_RIB_OBJECT_NAME + RIBObjectNames.SEPARATOR + stateName;
		
		CDAPMessage cdapMessage = cdapSessionManager.getReadObjectRequestMessage(portId, null, null,
				FlowStateObject.FLOW_STATE_RIB_OBJECT_CLASS, 0, objName, 0, false);

		this.ribDaemon.sendMessage(cdapMessage, portId , null);
		*/
		Timer timer = new Timer();
		timer.schedule(new ErrorCDAP(), waitUntilError);
	}
	
}
