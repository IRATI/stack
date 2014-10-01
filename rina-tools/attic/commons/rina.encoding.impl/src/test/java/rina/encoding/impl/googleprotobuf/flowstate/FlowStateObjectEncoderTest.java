package rina.encoding.impl.googleprotobuf.flowstate;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;

import rina.encoding.impl.googleprotobuf.flowstate.FlowStateEncoder;
import rina.pduftg.api.linkstate.FlowStateObject;

public class FlowStateObjectEncoderTest {

	private FlowStateObject flowStateObject = null;
	private FlowStateEncoder flowStateEncoder = null;
	
	@Before
	public void setup(){
		flowStateObject = new FlowStateObject(1,1,2,1,true,1,1);
		
		flowStateEncoder = new FlowStateEncoder();
	}
	
	@Test
	public void testSingle() throws Exception{
		byte[] serializedFSO = flowStateEncoder.encode(flowStateObject);
		for(int i=0; i<serializedFSO.length; i++){
			System.out.print(serializedFSO[i] + " ");
		}
		System.out.println();
		
		FlowStateObject recoveredFlowStateObject = (FlowStateObject) flowStateEncoder.decode(serializedFSO, FlowStateObject.class);
		
		Assert.assertEquals(flowStateObject.getAddress(), recoveredFlowStateObject.getAddress());
		Assert.assertEquals(flowStateObject.getNeighborAddress(), recoveredFlowStateObject.getNeighborAddress());
		Assert.assertEquals(flowStateObject.getPortid(), recoveredFlowStateObject.getPortid());
		Assert.assertEquals(flowStateObject.getNeighborPortid(), recoveredFlowStateObject.getNeighborPortid());
		Assert.assertEquals(flowStateObject.getSequenceNumber(), recoveredFlowStateObject.getSequenceNumber());
		Assert.assertEquals(flowStateObject.getAge(), recoveredFlowStateObject.getAge());
		Assert.assertEquals(flowStateObject.isState(), recoveredFlowStateObject.isState());
	}
}
