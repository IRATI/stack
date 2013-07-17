package rina.encoding.impl.googleprotobuf.flowservice;

import rina.encoding.api.BaseEncoder;
import rina.encoding.impl.googleprotobuf.GPBUtils;
import rina.applicationprocess.api.ApplicationProcessNamingInfo;
import rina.ipcservice.api.FlowService;
import rina.ipcservice.api.QualityOfServiceSpecification;

public class FlowServiceEncoder extends BaseEncoder{

	public synchronized Object decode(byte[] serializedObject, Class<?> objectClass) throws Exception {
		if (objectClass == null || !(objectClass.equals(FlowService.class))){
			throw new Exception("This is not the serializer for objects of type "+objectClass.getName());
		}
		
		FlowServiceMessage.FlowService gpbFlowService = FlowServiceMessage.FlowService.parseFrom(serializedObject);
		
		ApplicationProcessNamingInfo destinationAPName = GPBUtils.getApplicationProcessNamingInfo(gpbFlowService.getDestinationNamingInfo());
		ApplicationProcessNamingInfo sourceAPName = GPBUtils.getApplicationProcessNamingInfo(gpbFlowService.getSourceNamingInfo());
		QualityOfServiceSpecification qosSpec = GPBUtils.getQualityOfServiceSpecification(gpbFlowService.getQosSpecification());
		
		FlowService result = new FlowService();
		result.setSourceAPNamingInfo(sourceAPName);
		result.setDestinationAPNamingInfo(destinationAPName);
		result.setQoSSpecification(qosSpec);
		result.setPortId((int)gpbFlowService.getPortId());
		
		return result;
	}

	public synchronized byte[] encode(Object object) throws Exception {
		if (object == null || !(object instanceof FlowService)){
			throw new Exception("This is not the serializer for objects of type "+FlowService.class.toString());
		}

		FlowService flowService = (FlowService) object;
		FlowServiceMessage.FlowService gpbFlowService = null;

		if (flowService.getSourceAPNamingInfo() == null && flowService.getQoSSpecification() == null){
			gpbFlowService = FlowServiceMessage.FlowService.newBuilder().
			setDestinationNamingInfo(GPBUtils.getApplicationProcessNamingInfoT(flowService.getDestinationAPNamingInfo())).
			setPortId(flowService.getPortId()).build();
		}else if (flowService.getSourceAPNamingInfo() == null && flowService.getQoSSpecification() != null){
			gpbFlowService = FlowServiceMessage.FlowService.newBuilder().
			setDestinationNamingInfo(GPBUtils.getApplicationProcessNamingInfoT(flowService.getDestinationAPNamingInfo())).
			setQosSpecification(GPBUtils.getQoSSpecificationT(flowService.getQoSSpecification())).
			setPortId(flowService.getPortId()).build();
		}
		else if (flowService.getSourceAPNamingInfo() != null && flowService.getQoSSpecification() == null){
			gpbFlowService = FlowServiceMessage.FlowService.newBuilder().
			setDestinationNamingInfo(GPBUtils.getApplicationProcessNamingInfoT(flowService.getDestinationAPNamingInfo())).
			setSourceNamingInfo(GPBUtils.getApplicationProcessNamingInfoT(flowService.getSourceAPNamingInfo())).
			setPortId(flowService.getPortId()).build();
		}else{
			gpbFlowService = FlowServiceMessage.FlowService.newBuilder().
			setDestinationNamingInfo(GPBUtils.getApplicationProcessNamingInfoT(flowService.getDestinationAPNamingInfo())).
			setSourceNamingInfo(GPBUtils.getApplicationProcessNamingInfoT(flowService.getSourceAPNamingInfo())).
			setQosSpecification(GPBUtils.getQoSSpecificationT(flowService.getQoSSpecification())).
			setPortId(flowService.getPortId()).build();
		}

		return gpbFlowService.toByteArray();
	}

}
