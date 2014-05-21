package rina.encoding.impl.googleprotobuf.flowstate;

import rina.encoding.api.Encoder;
import rina.encoding.impl.googleprotobuf.flowstate.FlowStateMessage.flowStateObject_t;
import rina.pduftg.api.linkstate.FlowStateObject;

public class FlowStateEncoder implements Encoder{

	public synchronized Object decode(byte[] serializedObject, Class<?> objectClass) throws Exception {
		if (objectClass == null || !(objectClass.equals(FlowStateObject.class))){
			throw new Exception("This is not the encoder for objects of type "+objectClass.getName());
		}
		
		FlowStateMessage.flowStateObject_t gpbFlowState = FlowStateMessage.flowStateObject_t.parseFrom(serializedObject);
		return convertGPBToModel(gpbFlowState);
	}
	
	public static FlowStateObject convertGPBToModel(flowStateObject_t gpbFlowState){
		FlowStateObject flowState = new FlowStateObject(
				gpbFlowState.getAddress(),
				gpbFlowState.getPortid(),
				gpbFlowState.getNeighborAddress(),
				gpbFlowState.getNeighborPortid(),
				gpbFlowState.getState(),
				gpbFlowState.getSequenceNumber(),
				gpbFlowState.getAge());
		return flowState;
	}

	public synchronized byte[] encode(Object object) throws Exception {
		if (object == null || !(object instanceof FlowStateObject)){
			throw new Exception("This is not the encoder for objects of type " + FlowStateObject.class.toString());
		}
		
		FlowStateObject flowStateObject = (FlowStateObject) object;
		return convertModelToGPB(flowStateObject).toByteArray();
	}
	
	public static flowStateObject_t convertModelToGPB(FlowStateObject flowStateObject){
		FlowStateMessage.flowStateObject_t gpbFlowState = FlowStateMessage.flowStateObject_t.newBuilder().
				setAddress(flowStateObject.getAddress()).
				setPortid(flowStateObject.getPortid()).
				setNeighborAddress(flowStateObject.getNeighborAddress()).
				setNeighborPortid(flowStateObject.getPortid()).
				setState(flowStateObject.isState()).
				setSequenceNumber(flowStateObject.getSequenceNumber()).
				setAge(flowStateObject.getAge()).build();
		
		return gpbFlowState;
	}

}
