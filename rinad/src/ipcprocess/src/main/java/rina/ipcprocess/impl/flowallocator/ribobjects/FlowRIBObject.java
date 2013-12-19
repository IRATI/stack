package rina.ipcprocess.impl.flowallocator.ribobjects;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import rina.cdap.api.CDAPSessionDescriptor;
import rina.cdap.api.message.CDAPMessage;
import rina.flowallocator.api.FlowAllocatorInstance;
import rina.flowallocator.api.Flow;
import rina.ribdaemon.api.RIBDaemonException;
import rina.ribdaemon.api.SimpleSetMemberRIBObject;

public class FlowRIBObject extends SimpleSetMemberRIBObject{
	
	private static final Log log = LogFactory.getLog(FlowRIBObject.class);
	
	private FlowAllocatorInstance flowAllocatorInstance = null;
	
	public FlowRIBObject(String objectName, FlowAllocatorInstance flowAllocatorInstance){
		super(Flow.FLOW_RIB_OBJECT_CLASS, objectName, flowAllocatorInstance.getFlow());
		this.flowAllocatorInstance = flowAllocatorInstance;
	}
	
	@Override
	public void delete(CDAPMessage cdapMessage, CDAPSessionDescriptor cdapSessionDescriptor) throws RIBDaemonException{
		flowAllocatorInstance.deleteFlowRequestMessageReceived(cdapMessage, cdapSessionDescriptor.getPortId());
	}
	
	/**
	 * A new flow named as an already existing flow wants to be created. Send reply message with an error
	 */
	@Override
	public void create(CDAPMessage cdapMessage, CDAPSessionDescriptor cdapSessionDescriptor) throws RIBDaemonException{
		log.error("Requesting to create a new flow with an objectName that is already present in the RIB: "+cdapMessage.getObjName());
		try{
			CDAPMessage errorMessage = cdapMessage.getReplyMessage();
			errorMessage.setResult(-5);
			errorMessage.setResultReason("Requested to create a new flow with an object name that already exists in the RIB");
			this.getRIBDaemon().sendMessage(errorMessage, cdapSessionDescriptor.getPortId(), null);
		}catch(Exception ex){	
		}
	}
	
	@Override
	public Object getObjectValue(){
		return flowAllocatorInstance.getFlow();
	}
}
