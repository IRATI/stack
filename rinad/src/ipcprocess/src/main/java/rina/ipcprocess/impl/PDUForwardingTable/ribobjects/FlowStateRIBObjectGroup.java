package rina.ipcprocess.impl.PDUForwardingTable.ribobjects;


import rina.cdap.api.CDAPSessionDescriptor;
import rina.cdap.api.message.CDAPMessage;
import rina.ipcprocess.api.IPCProcess;
import rina.ipcprocess.impl.PDUForwardingTable.PDUFTable;
import rina.ribdaemon.api.BaseRIBObject;
import rina.ribdaemon.api.ObjectInstanceGenerator;
import rina.ribdaemon.api.RIBDaemonException;
import rina.ribdaemon.api.RIBObject;
import rina.PDUForwardingTable.api.FlowStateObjectGroup;

public class FlowStateRIBObjectGroup extends BaseRIBObject{
	
	PDUFTable pdufTable = null;
	
	/*		Constructors		*/
	public FlowStateRIBObjectGroup(PDUFTable table, IPCProcess ipcProcess)
	{
		super(ipcProcess, FlowStateObjectGroup.FLOW_STATE_GROUP_RIB_OBJECT_CLASS, 
				ObjectInstanceGenerator.getObjectInstance(), FlowStateObjectGroup.FLOW_STATE_GROUP_RIB_OBJECT_NAME);
		pdufTable = table;
	}
	@Override
	public Object getObjectValue(){
		String[] result = new String[this.getChildren().size()];
		for(int i=0; i<result.length; i++){
			result[i] = (String) this.getChildren().get(i).getObjectValue();
		}
		return result;
	}
	@Override
	public void write(CDAPMessage cdapMessage, CDAPSessionDescriptor cdapSessionDescriptor)
			throws RIBDaemonException 
	{
		pdufTable.writeMessageRecieved(cdapMessage, cdapSessionDescriptor.getPortId());
	}

}
