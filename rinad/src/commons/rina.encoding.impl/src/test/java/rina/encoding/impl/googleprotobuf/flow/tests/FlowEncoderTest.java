package rina.encoding.impl.googleprotobuf.flow.tests;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;

import rina.encoding.impl.googleprotobuf.flow.FlowEncoder;
import rina.flowallocator.api.ConnectionId;
import rina.flowallocator.api.Flow;
import rina.ipcprocess.api.IPCProcess;
import rina.applicationprocess.api.ApplicationProcessNamingInfo;
import rina.ipcservice.api.QualityOfServiceSpecification;

/**
 * Test if the serialization/deserialization mechanisms for the Flow object work
 * @author eduardgrasa
 *
 */
public class FlowEncoderTest {
	
	private Flow flow = null;
	private Flow flow2 = null;
	private FlowEncoder flowSerializer = null;
	private IPCProcess fakeIPCProcess = null;
	
	@Before
	public void setup(){
		fakeIPCProcess = new FakeIPCProcess();
		flowSerializer = new FlowEncoder();
		flowSerializer.setIPCProcess(fakeIPCProcess);
		
		flow = new Flow();
		flow.setAccessControl(new byte[]{0x01, 0x02, 0x03, 0x04});
		flow.setCreateFlowRetries(2);
		flow.setMaxCreateFlowRetries(3);
		flow.setDestinationAddress(1);
		flow.setDestinationNamingInfo(new ApplicationProcessNamingInfo("b", null));
		flow.setDestinationPortId(430);
		flow.setHopCount(3);
		List<ConnectionId> connectionIds = new ArrayList<ConnectionId>();
		ConnectionId connectionId = new ConnectionId();
		connectionId.setDestinationCEPId(43);
		connectionId.setSourceCEPId(55);
		connectionId.setQosId((byte)1);
		connectionIds.add(connectionId);
		flow.setConnectionIds(connectionIds);
		flow.setCurrentConnectionIdIndex(0);
		flow.setSourceAddress(2);
		flow.setSourceNamingInfo(new ApplicationProcessNamingInfo("a", null));
		flow.setSourcePortId(3327);
		Map<String, String> policies = new ConcurrentHashMap<String, String>();
		policies.put("policyname1", "policyvalue1");
		policies.put("policyname2", "policyvalue2");
		flow.setPolicies(policies);
		flow.setPolicyParameters(policies);
		
		flow2 = new Flow();
		flow2.setSourceAddress(1);
		flow2.setDestinationAddress(2);
		flow2.setDestinationNamingInfo(new ApplicationProcessNamingInfo("d", null));
		flow2.setSourceNamingInfo(new ApplicationProcessNamingInfo("c", null));
		flow2.setHopCount(1);
		QualityOfServiceSpecification qosSpec = new QualityOfServiceSpecification();
		qosSpec.setDelay(24);
		qosSpec.setAverageBandwidth(100000);
		flow2.setQosParameters(qosSpec);
	}
	
	@Test
	public void testSerilalization() throws Exception{
		byte[] serializedFlow = flowSerializer.encode(flow);
		for(int i=0; i<serializedFlow.length; i++){
			System.out.print(serializedFlow[i] + " ");
		}
		System.out.println("");
		
		Flow recoveredFlow = (Flow) flowSerializer.decode(serializedFlow, Flow.class);
		Assert.assertArrayEquals(flow.getAccessControl(), recoveredFlow.getAccessControl());
		Assert.assertEquals(flow.getCreateFlowRetries(), recoveredFlow.getCreateFlowRetries());
		Assert.assertEquals(flow.getCurrentConnectionIdIndex(), recoveredFlow.getCurrentConnectionIdIndex());
		Assert.assertEquals(flow.getDestinationAddress(), recoveredFlow.getDestinationAddress());
		Assert.assertEquals(flow.getDestinationNamingInfo(), recoveredFlow.getDestinationNamingInfo());
		Assert.assertEquals(flow.getDestinationPortId(), recoveredFlow.getDestinationPortId());
		Assert.assertEquals(flow.getHopCount(), recoveredFlow.getHopCount());
		Assert.assertEquals(flow.getConnectionIds().get(0).getDestinationCEPId(), recoveredFlow.getConnectionIds().get(0).getDestinationCEPId());
		Assert.assertEquals(flow.getConnectionIds().get(0).getSourceCEPId(), recoveredFlow.getConnectionIds().get(0).getSourceCEPId());
		Assert.assertEquals(flow.getConnectionIds().get(0).getQosId(), recoveredFlow.getConnectionIds().get(0).getQosId());
		Assert.assertEquals(flow.getSourceAddress(), recoveredFlow.getSourceAddress());
		Assert.assertEquals(flow.getSourceNamingInfo(), recoveredFlow.getSourceNamingInfo());
		Assert.assertEquals(flow.getSourcePortId(), recoveredFlow.getSourcePortId());
		Assert.assertEquals(flow.getPolicies().get("policyname1"), recoveredFlow.getPolicies().get("policyname1"));
		
		serializedFlow = flowSerializer.encode(flow2);
		for(int i=0; i<serializedFlow.length; i++){
			System.out.print(serializedFlow[i] + " ");
		}
		recoveredFlow = (Flow) flowSerializer.decode(serializedFlow, Flow.class);
		Assert.assertEquals(flow2.getDestinationAddress(), recoveredFlow.getDestinationAddress());
		Assert.assertEquals(flow2.getDestinationNamingInfo(), recoveredFlow.getDestinationNamingInfo());
		Assert.assertEquals(flow2.getHopCount(), recoveredFlow.getHopCount());
		Assert.assertEquals(flow2.getSourceAddress(), recoveredFlow.getSourceAddress());
		Assert.assertEquals(flow2.getSourceNamingInfo(), recoveredFlow.getSourceNamingInfo());
		Assert.assertEquals(flow2.getQosParameters().getAverageBandwidth(), recoveredFlow.getQosParameters().getAverageBandwidth());
	}

}
