package rina.ipcprocess.impl.flowallocator.policies;

import java.util.ArrayList;
import java.util.List;

import eu.irati.librina.Connection;
import eu.irati.librina.ConnectionPolicies;
import eu.irati.librina.DTCPConfig;
import eu.irati.librina.DTCPFlowControlConfig;
import eu.irati.librina.DTCPRateBasedFlowControlConfig;
import eu.irati.librina.DTCPRtxControlConfig;
import eu.irati.librina.DTCPWindowBasedFlowControlConfig;
import eu.irati.librina.PolicyConfig;
import eu.irati.librina.FlowRequestEvent;
import eu.irati.librina.IPCException;

import rina.flowallocator.api.Flow;
import rina.flowallocator.api.Flow.State;

public class NewFlowRequestPolicyImpl implements NewFlowRequestPolicy{

	public Flow generateFlowObject(FlowRequestEvent event, String difName) throws IPCException {
		Flow flow = new Flow();
		flow.setDestinationNamingInfo(event.getRemoteApplicationName());
		flow.setSourceNamingInfo(event.getLocalApplicationName());
		flow.setHopCount(3);
		flow.setMaxCreateFlowRetries(1);
		flow.setSource(true);
		flow.setState(State.ALLOCATION_IN_PROGRESS);
		List<Connection> connections = new ArrayList<Connection>();
		
		//TODO select qos cube properly
		int qosId = 1;
		Connection connection = new Connection();
		connection.setQosId(qosId);
		connection.setFlowUserIpcProcessId(event.getFlowRequestorIPCProcessId());
		connections.add(connection);
		flow.setConnections(connections);
		
		//TODO generate connection policies properly
		ConnectionPolicies connectionPolicies = new ConnectionPolicies();
		connectionPolicies.setDtcPpresent(true);
		connectionPolicies.setSeqnumrolloverthreshold(1234);
		connectionPolicies.setInitialseqnumpolicy(
				new PolicyConfig("policy1", "23"));
		DTCPConfig dtcpConfig = new DTCPConfig();
		dtcpConfig.setRtxcontrol(true);
		DTCPRtxControlConfig rtxConfig = new DTCPRtxControlConfig();
		rtxConfig.setDatarxmsnmax(25423);
		rtxConfig.setInitialATimer(14561);
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
		
		return flow;
	}

}
