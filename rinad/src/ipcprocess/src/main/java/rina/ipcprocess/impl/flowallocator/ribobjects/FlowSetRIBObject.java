package rina.ipcprocess.impl.flowallocator.ribobjects;

import rina.cdap.api.CDAPSessionDescriptor;
import rina.cdap.api.message.CDAPMessage;
import rina.flowallocator.api.FlowAllocator;
import rina.flowallocator.api.FlowAllocatorInstance;
import rina.flowallocator.api.Flow;
import rina.ipcprocess.impl.IPCProcess;
import rina.ribdaemon.api.BaseRIBObject;
import rina.ribdaemon.api.ObjectInstanceGenerator;
import rina.ribdaemon.api.RIBDaemonException;
import rina.ribdaemon.api.RIBObject;

public class FlowSetRIBObject extends BaseRIBObject{
	
	private FlowAllocator flowAllocator = null;
	private IPCProcess ipcProcess = null;
	
	public FlowSetRIBObject(FlowAllocator flowAllocator){
		super(Flow.FLOW_SET_RIB_OBJECT_CLASS, ObjectInstanceGenerator.getObjectInstance(), 
				Flow.FLOW_SET_RIB_OBJECT_NAME);
		this.flowAllocator = flowAllocator;
		this.ipcProcess = IPCProcess.getInstance();
		setRIBDaemon(ipcProcess.getRIBDaemon());
		setEncoder(ipcProcess.getEncoder());
	}
	
	/**
	 * A new Flow has to be created
	 */
	@Override
	public void create(CDAPMessage cdapMessage, CDAPSessionDescriptor cdapSessionDescriptor) throws RIBDaemonException{
		flowAllocator.createFlowRequestMessageReceived(cdapMessage, cdapSessionDescriptor.getPortId());
	}
	
	@Override
	public void create(String objectClass, long objectInstance, String objectName, Object object) throws RIBDaemonException{
		if (!(object instanceof FlowAllocatorInstance)){
			throw new RIBDaemonException(RIBDaemonException.OBJECTCLASS_DOES_NOT_MATCH_OBJECTNAME, 
					"Object class ("+object.getClass().getName()+") does not match object name "+objectName);
		}
		
		FlowRIBObject ribObject = new FlowRIBObject(objectName, (FlowAllocatorInstance) object);
		this.addChild(ribObject);
		getRIBDaemon().addRIBObject(ribObject);
	}
	
	@Override
	public RIBObject read() throws RIBDaemonException{
		return this;
	}
	
	@Override
	public Object getObjectValue(){
		Flow[] result = new Flow[this.getChildren().size()];
		for(int i=0; i<result.length; i++){
			result[i] = (Flow) this.getChildren().get(i).getObjectValue();
		}
		
		return result;
	}
}
