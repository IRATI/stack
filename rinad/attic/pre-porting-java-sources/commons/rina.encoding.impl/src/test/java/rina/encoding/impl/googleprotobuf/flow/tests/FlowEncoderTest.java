package rina.encoding.impl.googleprotobuf.flow.tests;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;

import rina.encoding.impl.googleprotobuf.flow.FlowEncoder;
import rina.flowallocator.api.Flow;
import eu.irati.librina.ApplicationProcessNamingInformation;
import eu.irati.librina.ConnectionPolicies;
import eu.irati.librina.DTCPConfig;
import eu.irati.librina.DTCPFlowControlConfig;
import eu.irati.librina.DTCPRateBasedFlowControlConfig;
import eu.irati.librina.DTCPRtxControlConfig;
import eu.irati.librina.DTCPWindowBasedFlowControlConfig;
import eu.irati.librina.PolicyConfig;

/**
 * Test if the serialization/deserialization mechanisms for the Flow object work
 * @author eduardgrasa
 *
 */
public class FlowEncoderTest {
	
	static {
		System.loadLibrary("rina_java");
	}
	
	private Flow flow = null;
	private FlowEncoder flowEncoder = null;
	
	@Before
	public void setup(){
		flow = new Flow();
		flow.setSourceNamingInfo(new ApplicationProcessNamingInformation("test", "1"));
		flow.setDestinationNamingInfo(new ApplicationProcessNamingInformation("test2", "1"));
		ConnectionPolicies connectionPolicies = new ConnectionPolicies();
		connectionPolicies.setDtcpPresent(true);
		connectionPolicies.setSeqnumrolloverthreshold(1234);
		connectionPolicies.setInitialATimer(14561);
		connectionPolicies.setInitialseqnumpolicy(
				new PolicyConfig("policy1", "23"));
		DTCPConfig dtcpConfig = new DTCPConfig();
		dtcpConfig.setRtxcontrol(true);
		DTCPRtxControlConfig rtxConfig = new DTCPRtxControlConfig();
		rtxConfig.setDatarxmsnmax(25423);
		dtcpConfig.setRtxcontrolconfig(rtxConfig);
		dtcpConfig.setFlowcontrol(true);
		DTCPFlowControlConfig flowConfig = new DTCPFlowControlConfig();
		flowConfig.setRcvbuffersthreshold(412431);
		flowConfig.setRcvbytespercentthreshold(134);
		flowConfig.setRcvbytesthreshold(46236);
		flowConfig.setSentbuffersthreshold(94);
		flowConfig.setSentbytespercentthreshold(2562);
		flowConfig.setSentbytesthreshold(26236);
		flowConfig.setWindowbased(true);
		DTCPWindowBasedFlowControlConfig window = new DTCPWindowBasedFlowControlConfig();
		window.setInitialcredit(62556);
		window.setMaxclosedwindowqueuelength(5612623);
		flowConfig.setWindowbasedconfig(window);
		flowConfig.setRatebased(true);
		DTCPRateBasedFlowControlConfig rate = new DTCPRateBasedFlowControlConfig();
		rate.setSendingrate(45125);
		rate.setTimeperiod(1451234);
		flowConfig.setRatebasedconfig(rate);
		dtcpConfig.setFlowcontrolconfig(flowConfig);
		dtcpConfig.setInitialrecvrinactivitytime(34);
		dtcpConfig.setInitialsenderinactivitytime(51245);
		connectionPolicies.setDtcpConfiguration(dtcpConfig);
		flow.setConnectionPolicies(connectionPolicies);
		flowEncoder = new FlowEncoder();
	}
	
	@Test
	public void testEncoding() throws Exception{
		byte[] encodedFlow = flowEncoder.encode(flow);
		for(int i=0; i<encodedFlow.length; i++){
			System.out.print(encodedFlow[i] + " ");
		}
		System.out.println();
		
		Flow recoveredFlow = (Flow) flowEncoder.decode(encodedFlow, Flow.class);
		Assert.assertEquals(flow.getDestinationAddress(), recoveredFlow.getDestinationAddress());
		Assert.assertEquals(flow.getSourceAddress(), recoveredFlow.getDestinationAddress());
		Assert.assertEquals(flow.getConnectionPolicies().isDtcpPresent(), 
				recoveredFlow.getConnectionPolicies().isDtcpPresent());
		Assert.assertEquals(flow.getConnectionPolicies().getInitialATimer(), 
				recoveredFlow.getConnectionPolicies().getInitialATimer());
		Assert.assertEquals(flow.getConnectionPolicies().getSeqnumrolloverthreshold(), 
				recoveredFlow.getConnectionPolicies().getSeqnumrolloverthreshold());
		Assert.assertEquals(flow.getConnectionPolicies().getInitialseqnumpolicy().getName(), 
				recoveredFlow.getConnectionPolicies().getInitialseqnumpolicy().getName());
		Assert.assertEquals(flow.getConnectionPolicies().getInitialseqnumpolicy().getVersion(), 
				recoveredFlow.getConnectionPolicies().getInitialseqnumpolicy().getVersion());
		Assert.assertEquals(flow.getConnectionPolicies().getDtcpConfiguration().isFlowcontrol(), 
				recoveredFlow.getConnectionPolicies().getDtcpConfiguration().isFlowcontrol());
		Assert.assertEquals(flow.getConnectionPolicies().getDtcpConfiguration().isRtxcontrol(), 
				recoveredFlow.getConnectionPolicies().getDtcpConfiguration().isRtxcontrol());
		Assert.assertEquals(flow.getConnectionPolicies().getDtcpConfiguration().getInitialrecvrinactivitytime(), 
				recoveredFlow.getConnectionPolicies().getDtcpConfiguration().getInitialrecvrinactivitytime());
		Assert.assertEquals(flow.getConnectionPolicies().getDtcpConfiguration().getInitialsenderinactivitytime(), 
				recoveredFlow.getConnectionPolicies().getDtcpConfiguration().getInitialsenderinactivitytime());
		Assert.assertEquals(flow.getConnectionPolicies().getDtcpConfiguration().getRtxcontrolconfig().getDatarxmsnmax(), 
				recoveredFlow.getConnectionPolicies().getDtcpConfiguration().getRtxcontrolconfig().getDatarxmsnmax());
		Assert.assertEquals(flow.getConnectionPolicies().getDtcpConfiguration().getFlowcontrolconfig().getRcvbuffersthreshold(), 
				recoveredFlow.getConnectionPolicies().getDtcpConfiguration().getFlowcontrolconfig().getRcvbuffersthreshold());
		Assert.assertEquals(flow.getConnectionPolicies().getDtcpConfiguration().getFlowcontrolconfig().getRcvbytespercentthreshold(), 
				recoveredFlow.getConnectionPolicies().getDtcpConfiguration().getFlowcontrolconfig().getRcvbytespercentthreshold());
		Assert.assertEquals(flow.getConnectionPolicies().getDtcpConfiguration().getFlowcontrolconfig().getRcvbytesthreshold(), 
				recoveredFlow.getConnectionPolicies().getDtcpConfiguration().getFlowcontrolconfig().getRcvbytesthreshold());
		Assert.assertEquals(flow.getConnectionPolicies().getDtcpConfiguration().getFlowcontrolconfig().getSentbuffersthreshold(), 
				recoveredFlow.getConnectionPolicies().getDtcpConfiguration().getFlowcontrolconfig().getSentbuffersthreshold());
		Assert.assertEquals(flow.getConnectionPolicies().getDtcpConfiguration().getFlowcontrolconfig().getSentbytespercentthreshold(), 
				recoveredFlow.getConnectionPolicies().getDtcpConfiguration().getFlowcontrolconfig().getSentbytespercentthreshold());
		Assert.assertEquals(flow.getConnectionPolicies().getDtcpConfiguration().getFlowcontrolconfig().getSentbytesthreshold(), 
				recoveredFlow.getConnectionPolicies().getDtcpConfiguration().getFlowcontrolconfig().getSentbytesthreshold());
		Assert.assertEquals(flow.getConnectionPolicies().getDtcpConfiguration().getFlowcontrolconfig().isWindowbased(), 
				recoveredFlow.getConnectionPolicies().getDtcpConfiguration().getFlowcontrolconfig().isWindowbased());
		Assert.assertEquals(flow.getConnectionPolicies().getDtcpConfiguration().getFlowcontrolconfig().isRatebased(), 
				recoveredFlow.getConnectionPolicies().getDtcpConfiguration().getFlowcontrolconfig().isRatebased());
		Assert.assertEquals(flow.getConnectionPolicies().getDtcpConfiguration().getFlowcontrolconfig().getWindowbasedconfig().getInitialcredit(), 
				recoveredFlow.getConnectionPolicies().getDtcpConfiguration().getFlowcontrolconfig().getWindowbasedconfig().getInitialcredit());
		Assert.assertEquals(flow.getConnectionPolicies().getDtcpConfiguration().getFlowcontrolconfig().getWindowbasedconfig().getMaxclosedwindowqueuelength(), 
				recoveredFlow.getConnectionPolicies().getDtcpConfiguration().getFlowcontrolconfig().getWindowbasedconfig().getMaxclosedwindowqueuelength());
		Assert.assertEquals(flow.getConnectionPolicies().getDtcpConfiguration().getFlowcontrolconfig().getRatebasedconfig().getSendingrate(), 
				recoveredFlow.getConnectionPolicies().getDtcpConfiguration().getFlowcontrolconfig().getRatebasedconfig().getSendingrate());
		Assert.assertEquals(flow.getConnectionPolicies().getDtcpConfiguration().getFlowcontrolconfig().getRatebasedconfig().getTimeperiod(), 
				recoveredFlow.getConnectionPolicies().getDtcpConfiguration().getFlowcontrolconfig().getRatebasedconfig().getTimeperiod());
	}

}
