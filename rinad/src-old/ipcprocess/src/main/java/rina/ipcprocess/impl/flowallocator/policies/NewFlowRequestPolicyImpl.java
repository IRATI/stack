package rina.ipcprocess.impl.flowallocator.policies;

import java.util.ArrayList;
import java.util.List;

import eu.irati.librina.Connection;
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
		
		int qosId = 1;
		Connection connection = new Connection();
		connection.setQosId(qosId);
		connection.setFlowUserIpcProcessId(event.getFlowRequestorIPCProcessId());
		connections.add(connection);
		flow.setConnections(connections);
		
		return flow;
	}

}
