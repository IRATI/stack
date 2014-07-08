package rina.ipcprocess.impl.flowallocator.policies;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import eu.irati.librina.Connection;
import eu.irati.librina.ConnectionPolicies;
import eu.irati.librina.FlowRequestEvent;
import eu.irati.librina.FlowSpecification;
import eu.irati.librina.IPCException;
import eu.irati.librina.QoSCube;
import eu.irati.librina.QoSCubeList;

import rina.flowallocator.api.Flow;
import rina.flowallocator.api.Flow.State;

public class NewFlowRequestPolicyImpl implements NewFlowRequestPolicy{
	
	private static final Log log = LogFactory.getLog(NewFlowRequestPolicyImpl.class);

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

		QoSCube qosCube = selectQoSCube(event.getFlowSpecification(), qosCubes);
		log.debug("Selected qosCube with name "+qosCube.getName()+ " and policies: + " +
				"\n"+ qosCube.getEfcpPolicies().toString());
		qosCube.getEfcpPolicies().setDtcpPresent(false);
		
		Connection connection = new Connection();
		//TODO hardcoded value, we don't deal with QoS yet
		connection.setQosId(1);
		connection.setFlowUserIpcProcessId(event.getFlowRequestorIPCProcessId());
		
		//TODO generate connection policies properly
		ConnectionPolicies connectionPolicies = qosCube.getEfcpPolicies();
		connectionPolicies.setInOrderDelivery(qosCube.isOrderedDelivery());
		connectionPolicies.setPartialDelivery(qosCube.isPartialDelivery());
		if (qosCube.getMaxAllowableGap() == -1) {
			connectionPolicies.setMaxSduGap(Integer.MAX_VALUE);
		} else {
			connectionPolicies.setMaxSduGap(qosCube.getMaxAllowableGap());
		}
		
		connection.setPolicies(connectionPolicies);
		connections.add(connection);
		
		flow.setConnections(connections);
		flow.setConnectionPolicies(connectionPolicies);
		flow.setFlowSpec(event.getFlowSpecification());
		
		return flow;
	}
	
	private QoSCube selectQoSCube(FlowSpecification flowSpec, QoSCubeList qosCubes) throws IPCException { 
		QoSCube result = null;
		
		if (flowSpec.getMaxAllowableGap() >= 0) {
			Iterator<QoSCube> cubesIterator = qosCubes.iterator();
			while(cubesIterator.hasNext()) {
				result = cubesIterator.next();
				if (result.getEfcpPolicies().isDtcpPresent()) {
					if (result.getEfcpPolicies().getDtcpConfiguration().isRtxcontrol()) {
						return result;
					}
				}
			}
			
			throw new IPCException("Could not find a QoS Cube with Rtx control enabled!");
		}

		return qosCubes.getFirst();
	}

}
