package rina.ipcprocess.impl.flowallocator.policies;

import java.util.ArrayList;
import java.util.List;

import eu.irati.librina.Connection;
import eu.irati.librina.ConnectionPolicies;
import eu.irati.librina.FlowRequestEvent;
import eu.irati.librina.IPCException;
import eu.irati.librina.QoSCube;
import eu.irati.librina.QoSCubeList;

import rina.flowallocator.api.Flow;
import rina.flowallocator.api.Flow.State;

public class NewFlowRequestPolicyImpl implements NewFlowRequestPolicy{

	public Flow generateFlowObject(FlowRequestEvent event, String difName, 
			QoSCubeList qosCubes) throws IPCException {
		Flow flow = new Flow();
		flow.setDestinationNamingInfo(event.getRemoteApplicationName());
		flow.setSourceNamingInfo(event.getLocalApplicationName());
		flow.setHopCount(3);
		flow.setMaxCreateFlowRetries(1);
		flow.setSource(true);
		flow.setState(State.ALLOCATION_IN_PROGRESS);
		List<Connection> connections = new ArrayList<Connection>();
		
		//TODO select qos cube properly
		QoSCube qosCube = qosCubes.getFirst();
		
		Connection connection = new Connection();
		connection.setQosId(qosCube.getId());
		connection.setFlowUserIpcProcessId(event.getFlowRequestorIPCProcessId());
		
		//TODO generate connection policies properly
		ConnectionPolicies connectionPolicies = qosCube.getEfcpPolicies();
		connectionPolicies.setInOrderDelivery(qosCube.isOrderedDelivery());
		connectionPolicies.setPartialDelivery(qosCube.isPartialDelivery());
		connectionPolicies.setMaxSduGap(qosCube.getMaxAllowableGap());
		
		connection.setPolicies(connectionPolicies);
		connections.add(connection);
		
		flow.setConnections(connections);
		flow.setConnectionPolicies(connectionPolicies);
		flow.setFlowSpec(event.getFlowSpecification());
		
		return flow;
	}

}
