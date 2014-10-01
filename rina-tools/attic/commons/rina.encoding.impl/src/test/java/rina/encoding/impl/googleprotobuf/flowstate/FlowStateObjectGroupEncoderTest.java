package rina.encoding.impl.googleprotobuf.flowstate;

import java.util.ArrayList;
import java.util.List;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;

import rina.pduftg.api.linkstate.FlowStateObject;
import rina.pduftg.api.linkstate.FlowStateObjectGroup;

public class FlowStateObjectGroupEncoderTest {

	private FlowStateObjectGroup flowStateObjectGroup = null;
	private FlowStateGroupEncoder flowStateGroupEncoder = null;
	
	@Before
	public void setup(){
		List<FlowStateObject> flowStateObjectList = new ArrayList<FlowStateObject>();
		FlowStateObject flowStateObject1 = new FlowStateObject(1,1,2,1,true,1,1);
		flowStateObjectList.add(flowStateObject1);
		
		FlowStateObject flowStateObject2 = new FlowStateObject(1,1,2,1,true,1,1);

		flowStateObjectList.add(flowStateObject2);
		
		flowStateObjectGroup = new FlowStateObjectGroup(flowStateObjectList);
		
		flowStateGroupEncoder = new FlowStateGroupEncoder();
	}
	
	@Test
	public void testSingle() throws Exception{
		byte[] serializedFSO = flowStateGroupEncoder.encode(flowStateObjectGroup);
		for(int i=0; i<serializedFSO.length; i++){
			System.out.print(serializedFSO[i] + " ");
		}
		System.out.println();
		
		FlowStateObjectGroup recoveredFlowStateObjectGroup = (FlowStateObjectGroup) flowStateGroupEncoder.decode(serializedFSO, FlowStateObjectGroup.class);
		
		Assert.assertEquals(flowStateObjectGroup.getFlowStateObjectArray().get(0).getAddress(), recoveredFlowStateObjectGroup.getFlowStateObjectArray().get(0).getAddress());
		Assert.assertEquals(flowStateObjectGroup.getFlowStateObjectArray().get(0).getPortid(), recoveredFlowStateObjectGroup.getFlowStateObjectArray().get(0).getPortid());
		Assert.assertEquals(flowStateObjectGroup.getFlowStateObjectArray().get(0).getNeighborAddress(), recoveredFlowStateObjectGroup.getFlowStateObjectArray().get(0).getNeighborAddress());
		Assert.assertEquals(flowStateObjectGroup.getFlowStateObjectArray().get(0).getNeighborPortid(), recoveredFlowStateObjectGroup.getFlowStateObjectArray().get(0).getNeighborPortid());
		Assert.assertEquals(flowStateObjectGroup.getFlowStateObjectArray().get(0).isState(), recoveredFlowStateObjectGroup.getFlowStateObjectArray().get(0).isState());
		Assert.assertEquals(flowStateObjectGroup.getFlowStateObjectArray().get(0).getSequenceNumber(), recoveredFlowStateObjectGroup.getFlowStateObjectArray().get(0).getSequenceNumber());
		Assert.assertEquals(flowStateObjectGroup.getFlowStateObjectArray().get(0).getAge(), recoveredFlowStateObjectGroup.getFlowStateObjectArray().get(0).getAge());
		
		Assert.assertEquals(flowStateObjectGroup.getFlowStateObjectArray().get(1).getAddress(), recoveredFlowStateObjectGroup.getFlowStateObjectArray().get(1).getAddress());
		Assert.assertEquals(flowStateObjectGroup.getFlowStateObjectArray().get(1).getPortid(), recoveredFlowStateObjectGroup.getFlowStateObjectArray().get(1).getPortid());
		Assert.assertEquals(flowStateObjectGroup.getFlowStateObjectArray().get(1).getNeighborAddress(), recoveredFlowStateObjectGroup.getFlowStateObjectArray().get(1).getNeighborAddress());
		Assert.assertEquals(flowStateObjectGroup.getFlowStateObjectArray().get(1).getNeighborPortid(), recoveredFlowStateObjectGroup.getFlowStateObjectArray().get(1).getNeighborPortid());
		Assert.assertEquals(flowStateObjectGroup.getFlowStateObjectArray().get(1).isState(), recoveredFlowStateObjectGroup.getFlowStateObjectArray().get(1).isState());
		Assert.assertEquals(flowStateObjectGroup.getFlowStateObjectArray().get(1).getSequenceNumber(), recoveredFlowStateObjectGroup.getFlowStateObjectArray().get(1).getSequenceNumber());
		Assert.assertEquals(flowStateObjectGroup.getFlowStateObjectArray().get(1).getAge(), recoveredFlowStateObjectGroup.getFlowStateObjectArray().get(1).getAge());	
	}
}
