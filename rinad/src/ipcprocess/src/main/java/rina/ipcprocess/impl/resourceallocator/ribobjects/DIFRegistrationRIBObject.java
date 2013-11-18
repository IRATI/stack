package rina.ipcprocess.impl.resourceallocator.ribobjects;

import rina.cdap.api.CDAPSessionDescriptor;
import rina.cdap.api.message.CDAPMessage;
import rina.resourceallocator.api.NMinus1FlowManager;
import rina.ribdaemon.api.RIBDaemonException;
import rina.ribdaemon.api.SimpleSetMemberRIBObject;

public class DIFRegistrationRIBObject extends SimpleSetMemberRIBObject{
	
	public static final String DIF_REGISTRATION_RIB_OBJECT_CLASS = "dif registration";
	
	private String difName = null;
	private NMinus1FlowManager nMinus1FlowManager = null;
	
	public DIFRegistrationRIBObject(String objectName, String difName, 
			NMinus1FlowManager nMinus1FlowManager){
		super(DIF_REGISTRATION_RIB_OBJECT_CLASS, objectName, difName);
		this.difName = difName;
		this.nMinus1FlowManager = nMinus1FlowManager;
	}
	
	@Override
	public void delete(CDAPMessage cdapMessage, CDAPSessionDescriptor cdapSessionDescriptor) throws RIBDaemonException{
		//TODO implement this by calling the N Minus 1 Flow Manager
	}
	
	@Override
	public Object getObjectValue(){
		return difName;
	}
}
