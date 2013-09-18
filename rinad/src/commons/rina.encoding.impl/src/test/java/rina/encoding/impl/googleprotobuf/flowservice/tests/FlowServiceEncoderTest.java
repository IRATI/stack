package rina.encoding.impl.googleprotobuf.flowservice.tests;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;

import rina.encoding.impl.googleprotobuf.flowservice.FlowServiceEncoder;
import rina.applicationprocess.api.ApplicationProcessNamingInfo;
import rina.ipcservice.api.FlowService;
import rina.ipcservice.api.QualityOfServiceSpecification;

/**
 * Test if the serialization/deserialization mechanisms for the FlowService object work
 * @author eduardgrasa
 *
 */
public class FlowServiceEncoderTest {
	
	private FlowService flowService = null;
	private FlowService flowService2 = null;
	private FlowService flowService3 = null;
	private FlowService flowService4 = null;
	private FlowServiceEncoder flowServiceEncoder = null;
	
	@Before
	public void setup(){
		flowServiceEncoder = new FlowServiceEncoder();
		
		flowService = new FlowService();
		flowService.setSourceAPNamingInfo(new ApplicationProcessNamingInfo("a", "1"));
		flowService.setDestinationAPNamingInfo(new ApplicationProcessNamingInfo("b", "1"));
		flowService.setPortId(3327);
		
		flowService2 = new FlowService();
		flowService2.setDestinationAPNamingInfo(new ApplicationProcessNamingInfo("d", "1"));
		flowService2.setPortId(33223);
		
		flowService3 = new FlowService();
		flowService3.setDestinationAPNamingInfo(new ApplicationProcessNamingInfo("d", "1"));
		QualityOfServiceSpecification qosSpec = new QualityOfServiceSpecification();
		qosSpec.setDelay(24);
		qosSpec.setAverageBandwidth(100000);
		flowService3.setQoSSpecification(qosSpec);
		flowService3.setPortId(33223);
		
		flowService4 = new FlowService();
		flowService4.setSourceAPNamingInfo(new ApplicationProcessNamingInfo("a", "1"));
		flowService4.setDestinationAPNamingInfo(new ApplicationProcessNamingInfo("b", "1"));
		qosSpec = new QualityOfServiceSpecification();
		qosSpec.setName("reliable");
		qosSpec.setQosCubeId(2);
		qosSpec.setJitter(24);
		qosSpec.setAverageSDUBandwidth(5400);
		qosSpec.setPeakSDUBandwidthDuration(23);
		flowService4.setQoSSpecification(qosSpec);
		flowService4.setPortId(3327);
	}
	
	@Test
	public void testSerilalization() throws Exception{
		byte[] encodedRequest = flowServiceEncoder.encode(flowService);
		for(int i=0; i<encodedRequest.length; i++){
			System.out.print(encodedRequest[i] + " ");
		}
		System.out.println("");
		
		FlowService recoveredRequest= (FlowService) flowServiceEncoder.decode(encodedRequest, FlowService.class);
		Assert.assertEquals(flowService.getPortId(), recoveredRequest.getPortId());
		Assert.assertEquals(flowService.getSourceAPNamingInfo(), recoveredRequest.getSourceAPNamingInfo());
		Assert.assertEquals(flowService.getDestinationAPNamingInfo(), recoveredRequest.getDestinationAPNamingInfo());
		
		encodedRequest = flowServiceEncoder.encode(flowService2);
		for(int i=0; i<encodedRequest.length; i++){
			System.out.print(encodedRequest[i] + " ");
		}
		System.out.println("");
		
		recoveredRequest= (FlowService) flowServiceEncoder.decode(encodedRequest, FlowService.class);
		System.out.println(recoveredRequest.toString());
		Assert.assertEquals(flowService2.getPortId(), recoveredRequest.getPortId());
		Assert.assertEquals(flowService2.getDestinationAPNamingInfo(), recoveredRequest.getDestinationAPNamingInfo());
		
		encodedRequest = flowServiceEncoder.encode(flowService3);
		for(int i=0; i<encodedRequest.length; i++){
			System.out.print(encodedRequest[i] + " ");
		}
		System.out.println("");
		
		recoveredRequest= (FlowService) flowServiceEncoder.decode(encodedRequest, FlowService.class);
		System.out.println(recoveredRequest.toString());
		Assert.assertEquals(flowService3.getPortId(), recoveredRequest.getPortId());
		Assert.assertEquals(flowService3.getDestinationAPNamingInfo(), recoveredRequest.getDestinationAPNamingInfo());
		Assert.assertEquals(flowService3.getQoSSpecification().getAverageBandwidth(), recoveredRequest.getQoSSpecification().getAverageBandwidth());
		
		encodedRequest = flowServiceEncoder.encode(flowService4);
		for(int i=0; i<encodedRequest.length; i++){
			System.out.print(encodedRequest[i] + " ");
		}
		System.out.println("");
		
		recoveredRequest= (FlowService) flowServiceEncoder.decode(encodedRequest, FlowService.class);
		System.out.println(recoveredRequest.toString());
		Assert.assertEquals(flowService4.getPortId(), recoveredRequest.getPortId());
		Assert.assertEquals(flowService4.getDestinationAPNamingInfo(), recoveredRequest.getDestinationAPNamingInfo());
		Assert.assertEquals(flowService4.getQoSSpecification().getAverageSDUBandwidth(), recoveredRequest.getQoSSpecification().getAverageSDUBandwidth());
		Assert.assertEquals(2, recoveredRequest.getQoSSpecification().getQosCubeId());
	}

}
