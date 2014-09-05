package rina.encoding.impl.googleprotobuf.flowservice;

import eu.irati.librina.ApplicationProcessNamingInformation;
import eu.irati.librina.FlowInformation;
import eu.irati.librina.FlowSpecification;
import rina.encoding.api.Encoder;
import rina.encoding.impl.googleprotobuf.GPBUtils;

public class FlowServiceEncoder implements Encoder{

	public synchronized Object decode(byte[] serializedObject, Class<?> objectClass) throws Exception {
		if (objectClass == null || !(objectClass.equals(FlowInformation.class))){
			throw new Exception("This is not the serializer for objects of type "+
					objectClass.getName());
		}
		
		FlowServiceMessage.FlowService gpbFlowService = 
				FlowServiceMessage.FlowService.parseFrom(serializedObject);
		
		ApplicationProcessNamingInformation destinationAPName = 
				GPBUtils.getApplicationProcessNamingInfo(
						gpbFlowService.getDestinationNamingInfo());
		ApplicationProcessNamingInformation sourceAPName = 
				GPBUtils.getApplicationProcessNamingInfo(
						gpbFlowService.getSourceNamingInfo());
		FlowSpecification flowSpec = GPBUtils.getFlowSpecification(
				gpbFlowService.getQosSpecification());
		
		FlowInformation result = new FlowInformation();
		result.setLocalAppName(sourceAPName);
		result.setRemoteAppName(destinationAPName);
		result.setFlowSpecification(flowSpec);
		result.setPortId((int)gpbFlowService.getPortId());
		
		return result;
	}

	public synchronized byte[] encode(Object object) throws Exception {
		if (object == null || !(object instanceof FlowInformation)){
			throw new Exception("This is not the serializer for objects of type "
					+FlowInformation.class.toString());
		}

		FlowInformation flowInformation = (FlowInformation) object;
		FlowServiceMessage.FlowService gpbFlowService = null;

		if (flowInformation.getLocalAppName() == null && flowInformation.getFlowSpecification() == null){
			gpbFlowService = FlowServiceMessage.FlowService.newBuilder().
			setDestinationNamingInfo(GPBUtils.getApplicationProcessNamingInfoT(flowInformation.getRemoteAppName())).
			setPortId(flowInformation.getPortId()).build();
		}else if (flowInformation.getLocalAppName() == null && flowInformation.getFlowSpecification() != null){
			gpbFlowService = FlowServiceMessage.FlowService.newBuilder().
			setDestinationNamingInfo(GPBUtils.getApplicationProcessNamingInfoT(flowInformation.getRemoteAppName())).
			setQosSpecification(GPBUtils.getQoSSpecificationT(flowInformation.getFlowSpecification())).
			setPortId(flowInformation.getPortId()).build();
		}else if (flowInformation.getLocalAppName() != null && flowInformation.getFlowSpecification() == null){
			gpbFlowService = FlowServiceMessage.FlowService.newBuilder().
			setDestinationNamingInfo(GPBUtils.getApplicationProcessNamingInfoT(
					flowInformation.getRemoteAppName())).
			setSourceNamingInfo(GPBUtils.getApplicationProcessNamingInfoT(
					flowInformation.getLocalAppName())).
			setPortId(flowInformation.getPortId()).build();
		}else{
			gpbFlowService = FlowServiceMessage.FlowService.newBuilder().
			setDestinationNamingInfo(GPBUtils.getApplicationProcessNamingInfoT(
					flowInformation.getRemoteAppName())).
			setSourceNamingInfo(GPBUtils.getApplicationProcessNamingInfoT(
					flowInformation.getLocalAppName())).
			setQosSpecification(GPBUtils.getQoSSpecificationT(
					flowInformation.getFlowSpecification())).
			setPortId(flowInformation.getPortId()).build();
		}

		return gpbFlowService.toByteArray();
	}

}
