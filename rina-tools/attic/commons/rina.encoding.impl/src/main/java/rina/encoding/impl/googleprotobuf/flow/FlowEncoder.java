package rina.encoding.impl.googleprotobuf.flow;

import java.util.ArrayList;
import java.util.List;

import eu.irati.librina.Connection;
import eu.irati.librina.ConnectionPolicies;
import eu.irati.librina.FlowSpecification;

import rina.encoding.api.Encoder;
import rina.encoding.impl.googleprotobuf.GPBUtils;
import rina.encoding.impl.googleprotobuf.flow.FlowMessage.connectionId_t;
import rina.encoding.impl.googleprotobuf.qosspecification.QoSSpecification.qosSpecification_t;
import rina.flowallocator.api.Flow;
import rina.flowallocator.api.Flow.State;

/**
 * Serializes, unserializes Flow objects using the GPB encoding
 * @author eduardgrasa
 *
 */
public class FlowEncoder implements Encoder{

	public synchronized Object decode(byte[] serializedObject, Class<?> objectClass) throws Exception {
		if (objectClass == null || !(objectClass.equals(Flow.class))){
			throw new Exception("This is not the serializer for objects of type "+objectClass.getName());
		}
		
		FlowMessage.Flow gpbFlow = FlowMessage.Flow.parseFrom(serializedObject);
		
		Flow flow = new Flow();
		flow.setConnections(getConnectionIds(gpbFlow.getConnectionIdsList()));
		flow.setAccessControl(GPBUtils.getByteArray(gpbFlow.getAccessControl()));
		flow.setCreateFlowRetries(gpbFlow.getCreateFlowRetries());
		flow.setCurrentConnectionIndex(gpbFlow.getCurrentConnectionIdIndex());
		flow.setDestinationAddress(gpbFlow.getDestinationAddress());
		flow.setDestinationNamingInfo(GPBUtils.getApplicationProcessNamingInfo(gpbFlow.getDestinationNamingInfo()));
		flow.setDestinationPortId((int)gpbFlow.getDestinationPortId());
		flow.setHopCount(gpbFlow.getHopCount());
		flow.setMaxCreateFlowRetries(gpbFlow.getMaxCreateFlowRetries());
		flow.setCreateFlowRetries(gpbFlow.getCreateFlowRetries());
		
		qosSpecification_t qosParams = gpbFlow.getQosParameters();
		if (!qosParams.equals(qosSpecification_t.getDefaultInstance())){
			flow.setFlowSpec(GPBUtils.getFlowSpecification(qosParams));
		} else {
			flow.setFlowSpec(new FlowSpecification());
		}
		
		FlowSpecification flowSpec = flow.getFlowSpec();
		ConnectionPolicies connectionPolicies = GPBUtils.getConnectionPolicies(gpbFlow.getConnectionPolicies());
		connectionPolicies.setInOrderDelivery(flowSpec.isOrderedDelivery());
		connectionPolicies.setPartialDelivery(flowSpec.isPartialDelivery());
		connectionPolicies.setMaxSduGap(flowSpec.getMaxAllowableGap());
		flow.setConnectionPolicies(connectionPolicies);
		for(int i=0; i<flow.getConnections().size(); i++){
			flow.getConnections().get(i).setPolicies(connectionPolicies);
		}
		
		flow.setSourceAddress(gpbFlow.getSourceAddress());
		flow.setSourceNamingInfo(GPBUtils.getApplicationProcessNamingInfo(gpbFlow.getSourceNamingInfo()));
		flow.setSourcePortId((int)gpbFlow.getSourcePortId());
		flow.setState(getFlowState(gpbFlow.getState()));
		flow.setHopCount(gpbFlow.getHopCount());
		
		return flow;
	}

	private List<Connection> getConnectionIds(List<connectionId_t> connectionIdsList) {
		List<Connection> result = new ArrayList<Connection>();
		
		for (int i=0; i<connectionIdsList.size(); i++){
			result.add(getConnectionId(connectionIdsList.get(i)));
		}
		
		return result;
	}
	
	private State getFlowState(int state){
		switch(state){
		case 0:
			return State.NULL;
		case 1:
			return State.ALLOCATION_IN_PROGRESS;
		case 2:
			return State.ALLOCATED;
		case 3:
			return State.WAITING_2_MPL_BEFORE_TEARING_DOWN;
		case 4:
			return State.DEALLOCATED;
		default:
			return State.NULL;
		}
	}
	
	private Connection getConnectionId(connectionId_t connectionId){
		Connection result = new Connection();
		
		result.setDestCepId(connectionId.getDestinationCEPId());
		result.setQosId(connectionId.getQosId());
		result.setSourceCepId(connectionId.getSourceCEPId());
		return result;
	}
	
	
	
	public synchronized byte[] encode(Object object) throws Exception {
		if (object == null || !(object instanceof Flow)){
			throw new Exception("This is not the serializer for objects of type "+Flow.class.toString());
		}
		
		Flow flow = (Flow) object;
		qosSpecification_t qosSpecificationT = GPBUtils.getQoSSpecificationT(flow.getFlowSpec());
		if (qosSpecificationT == null){
			qosSpecificationT = qosSpecification_t.getDefaultInstance();
		}
		
		FlowMessage.Flow gpbFlow = FlowMessage.Flow.newBuilder().
			setAccessControl(GPBUtils.getByteString(flow.getAccessControl())).
			setCreateFlowRetries(flow.getCreateFlowRetries()).
			setCurrentConnectionIdIndex(flow.getCurrentConnectionIndex()).
			addAllConnectionIds(getConnectionIdTypes(flow.getConnections())).
			setDestinationAddress(flow.getDestinationAddress()).
			setDestinationNamingInfo(GPBUtils.getApplicationProcessNamingInfoT(flow.getDestinationNamingInfo())).
			setDestinationPortId(flow.getDestinationPortId()).
			setHopCount(flow.getHopCount()).
			setConnectionPolicies(GPBUtils.getConnectionPoliciesType(flow.getConnectionPolicies())).
			setQosParameters(qosSpecificationT).
			setMaxCreateFlowRetries(flow.getMaxCreateFlowRetries()).
			setSourceAddress(flow.getSourceAddress()).
			setSourceNamingInfo(GPBUtils.getApplicationProcessNamingInfoT(flow.getSourceNamingInfo())).
			setSourcePortId(flow.getSourcePortId()).
			setState(getGPBState(flow.getState())).
			setHopCount(flow.getHopCount()).
			build();

		return gpbFlow.toByteArray();
	}
	
	private int getGPBState(State state){
		switch(state){
		case NULL:
			return 0;
		case ALLOCATION_IN_PROGRESS:
			return 1;
		case ALLOCATED:
			return 2;
		case WAITING_2_MPL_BEFORE_TEARING_DOWN:
			return 4;
		case DEALLOCATED:
			return 4;
		default:
			return 0;
		}
	}
	
	private List<connectionId_t> getConnectionIdTypes(List<Connection> connections){
		List<connectionId_t> result = new ArrayList<connectionId_t>();
		
		if (connections == null){
			return result;
		}
		
		for (int i=0; i<connections.size(); i++){
			result.add(getConnectionIdType(connections.get(i)));
		}
		
		return result;
	}
	
	private connectionId_t getConnectionIdType(Connection connection){
		if (connection == null){
			return null;
		}
		
		connectionId_t result = FlowMessage.connectionId_t.newBuilder().
									setDestinationCEPId(connection.getDestCepId()).
									setQosId((int)connection.getQosId()).
									setSourceCEPId(connection.getSourceCepId()).
									build();
		
		return result;
	}
}
