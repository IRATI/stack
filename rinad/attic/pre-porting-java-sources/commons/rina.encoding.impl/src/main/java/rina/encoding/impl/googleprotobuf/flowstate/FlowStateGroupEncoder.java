package rina.encoding.impl.googleprotobuf.flowstate;

import java.util.ArrayList;
import java.util.List;

import rina.encoding.api.Encoder;
import rina.encoding.impl.googleprotobuf.flowstate.FlowStateGroupMessage.flowStateObjectGroup_t;
import rina.encoding.impl.googleprotobuf.flowstate.FlowStateMessage.flowStateObject_t;
import rina.encoding.impl.googleprotobuf.flowstate.FlowStateGroupMessage.flowStateObjectGroup_t.Builder;
import rina.encoding.impl.googleprotobuf.flowstate.FlowStateEncoder;
import rina.pduftg.api.linkstate.FlowStateObject;
import rina.pduftg.api.linkstate.FlowStateObjectGroup;

public class FlowStateGroupEncoder implements Encoder{
	public synchronized Object decode(byte[] serializedObject, Class<?> objectClass) throws Exception {
		if (objectClass == null || !(objectClass.equals(FlowStateObjectGroup.class))){
			throw new Exception("This is not the encoder for objects of type "+objectClass.getName());
		}
		
		FlowStateGroupMessage.flowStateObjectGroup_t gpbFlowStateGroup = FlowStateGroupMessage.flowStateObjectGroup_t.parseFrom(serializedObject);
		return convertGPBToModel(gpbFlowStateGroup);
	}
	
	public static FlowStateObjectGroup convertGPBToModel(flowStateObjectGroup_t gpbFlowStateGroup){
		List<FlowStateObject> objectList = new ArrayList<FlowStateObject>();
		
		for (flowStateObject_t obj: gpbFlowStateGroup.getFlowStateObjectsList())
		{
			objectList.add(FlowStateEncoder.convertGPBToModel(obj));
		}
		return new FlowStateObjectGroup(objectList);
	}

	public synchronized byte[] encode(Object object) throws Exception {
		if (object == null || !(object instanceof FlowStateObjectGroup)){
			throw new Exception("This is not the encoder for objects of type " + FlowStateObjectGroup.class.toString());
		}
		
		FlowStateObjectGroup flowStateObjectGroup = (FlowStateObjectGroup) object;
		return convertModelToGPB(flowStateObjectGroup).toByteArray();
	}
	
	public static flowStateObjectGroup_t convertModelToGPB(FlowStateObjectGroup flowStateObjectGroup){
		Builder builder = FlowStateGroupMessage.flowStateObjectGroup_t.newBuilder();
		
		for (int i = 0; i < flowStateObjectGroup.getFlowStateObjectArray().size(); i++)
		{
			FlowStateObject obj = flowStateObjectGroup.getFlowStateObjectArray().get(i);
			builder.addFlowStateObjects(FlowStateEncoder.convertModelToGPB(obj));
		}

		
		return builder.build();
	}
}
