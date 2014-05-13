package rina.ipcprocess.impl.resourceallocator.ribobjects;

import rina.cdap.api.CDAPSessionDescriptor;
import rina.cdap.api.message.CDAPMessage;
import rina.ipcprocess.api.IPCProcess;
import rina.resourceallocator.api.NMinus1FlowManager;
import rina.ribdaemon.api.BaseRIBObject;
import rina.ribdaemon.api.ObjectInstanceGenerator;
import rina.ribdaemon.api.RIBDaemonException;
import rina.ribdaemon.api.RIBObject;
import rina.ribdaemon.api.RIBObjectNames;

public class DIFRegistrationSetRIBObject extends BaseRIBObject{
	
	public static final String DIF_REGISTRATION_SET_RIB_OBJECT_CLASS = "DIF registration set";
	public static final String DIF_REGISTRATION_SET_RIB_OBJECT_NAME = RIBObjectNames.SEPARATOR + RIBObjectNames.DIF + 
		RIBObjectNames.SEPARATOR + RIBObjectNames.RESOURCE_ALLOCATION + RIBObjectNames.SEPARATOR + 
		RIBObjectNames.NMINUSONEFLOWMANAGER + RIBObjectNames.SEPARATOR + RIBObjectNames.DIF_REGISTRATIONS;
	
	private NMinus1FlowManager nMinus1FlowManager = null;
	
	public DIFRegistrationSetRIBObject(IPCProcess ipcProcess, NMinus1FlowManager nMinus1FlowManager){
		super(ipcProcess, DIF_REGISTRATION_SET_RIB_OBJECT_CLASS, ObjectInstanceGenerator.getObjectInstance(), 
				DIF_REGISTRATION_SET_RIB_OBJECT_NAME);
		this.nMinus1FlowManager = nMinus1FlowManager;
		setRIBDaemon(ipcProcess.getRIBDaemon());
	}
	
	/**
	 * A new N-1 flow has to be allocated
	 */
	@Override
	public void create(CDAPMessage cdapMessage, CDAPSessionDescriptor cdapSessionDescriptor) throws RIBDaemonException{
		//TODO implement this (call nMinus1FlowManager)
	}
	
	@Override
	public void create(String objectClass, long objectInstance, String objectName, Object object) throws RIBDaemonException{
		if (!(object instanceof String)){
			throw new RIBDaemonException(RIBDaemonException.OBJECTCLASS_DOES_NOT_MATCH_OBJECTNAME, 
					"Object class ("+object.getClass().getName()+") does not match object name "+objectName);
		}
		
		DIFRegistrationRIBObject ribObject = new DIFRegistrationRIBObject(getIPCProcess(), objectName, 
				(String) object, this.nMinus1FlowManager);
		this.addChild(ribObject);
		getRIBDaemon().addRIBObject(ribObject);
	}
	
	@Override
	public RIBObject read() throws RIBDaemonException{
		return this;
	}
	
	@Override
	public Object getObjectValue(){
		String[] result = new String[this.getChildren().size()];
		for(int i=0; i<result.length; i++){
			result[i] = (String) this.getChildren().get(i).getObjectValue();
		}
		
		return result;
	}
}
