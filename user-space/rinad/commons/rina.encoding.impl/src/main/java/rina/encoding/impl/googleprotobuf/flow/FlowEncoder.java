package rina.encoding.impl.googleprotobuf.flow;

import java.util.ArrayList;
import java.util.List;

import rina.encoding.api.BaseEncoder;
import rina.encoding.impl.googleprotobuf.GPBUtils;
import rina.encoding.impl.googleprotobuf.flow.FlowMessage.connectionId_t;
import rina.encoding.impl.googleprotobuf.qosspecification.QoSSpecification.qosSpecification_t;
import rina.flowallocator.api.ConnectionId;
import rina.flowallocator.api.Flow;
import rina.flowallocator.api.Flow.State;

/**
 * Serializes, unserializes Flow objects using the GPB encoding
 * @author eduardgrasa
 *
 */
public class FlowEncoder extends BaseEncoder{

	public synchronized Object decode(byte[] serializedObject, Class<?> objectClass) throws Exception {
		if (objectClass == null || !(objectClass.equals(Flow.class))){
			throw new Exception("This is not the serializer for objects of type "+objectClass.getName());
		}
		
		FlowMessage.Flow gpbFlow = FlowMessage.Flow.parseFrom(serializedObject);
		
		Flow flow = new Flow();
		flow.setAccessControl(GPBUtils.getByteArray(gpbFlow.getAccessControl()));
		flow.setCreateFlowRetries(gpbFlow.getCreateFlowRetries());
		flow.setCurrentConnectionIdIndex(gpbFlow.getCurrentConnectionIdIndex());
		flow.setDestinationAddress(gpbFlow.getDestinationAddress());
		flow.setDestinationNamingInfo(GPBUtils.getApplicationProcessNamingInfo(gpbFlow.getDestinationNamingInfo()));
		flow.setDestinationPortId(gpbFlow.getDestinationPortId());
		flow.setConnectionIds(getConnectionIds(gpbFlow.getConnectionIdsList()));
		flow.setHopCount(gpbFlow.getHopCount());
		flow.setMaxCreateFlowRetries(gpbFlow.getMaxCreateFlowRetries());
		flow.setCreateFlowRetries(gpbFlow.getCreateFlowRetries());
		flow.setPolicies(GPBUtils.getProperties(gpbFlow.getPoliciesList()));
		flow.setPolicyParameters(GPBUtils.getProperties(gpbFlow.getPolicyParemetersList()));
		qosSpecification_t qosParams = gpbFlow.getQosParameters();
		if (!qosParams.equals(qosSpecification_t.getDefaultInstance())){
			flow.setQosParameters(GPBUtils.getQualityOfServiceSpecification(qosParams));
		}
		flow.setSourceAddress(gpbFlow.getSourceAddress());
		flow.setSourceNamingInfo(GPBUtils.getApplicationProcessNamingInfo(gpbFlow.getSourceNamingInfo()));
		flow.setSourcePortId(gpbFlow.getSourcePortId());
		flow.setState(getFlowState(gpbFlow.getState()));
		flow.setHopCount(gpbFlow.getHopCount());
		
		return flow;
	}

	private List<ConnectionId> getConnectionIds(List<connectionId_t> connectionIdsList) {
		List<ConnectionId> result = new ArrayList<ConnectionId>();
		
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
	
	private ConnectionId getConnectionId(connectionId_t connectionId){
		ConnectionId result = new ConnectionId();
		
		result.setDestinationCEPId(connectionId.getDestinationCEPId());
		result.setQosId(connectionId.getQosId());
		result.setSourceCEPId(connectionId.getSourceCEPId());
		return result;
	}
	 
	
	public synchronized byte[] encode(Object object) throws Exception {
		if (object == null || !(object instanceof Flow)){
			throw new Exception("This is not the serializer for objects of type "+Flow.class.toString());
		}
		
		Flow flow = (Flow) object;
		qosSpecification_t qosSpecificationT = GPBUtils.getQoSSpecificationT(flow.getQosParameters());
		if (qosSpecificationT == null){
			qosSpecificationT = qosSpecification_t.getDefaultInstance();
		}
		
		FlowMessage.Flow gpbFlow = FlowMessage.Flow.newBuilder().
			setAccessControl(GPBUtils.getByteString(flow.getAccessControl())).
			setCreateFlowRetries(flow.getCreateFlowRetries()).
			setCurrentConnectionIdIndex(flow.getCurrentConnectionIdIndex()).
			addAllConnectionIds(getConnectionIdTypes(flow.getConnectionIds())).
			setDestinationAddress(flow.getDestinationAddress()).
			setDestinationNamingInfo(GPBUtils.getApplicationProcessNamingInfoT(flow.getDestinationNamingInfo())).
			setDestinationPortId(flow.getDestinationPortId()).
			setHopCount(flow.getHopCount()).
			addAllPolicies(GPBUtils.getProperties(flow.getPolicies())).
			addAllPolicyParemeters(GPBUtils.getProperties(flow.getPolicyParameters())).
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
	
	private List<connectionId_t> getConnectionIdTypes(List<ConnectionId> connectionIds){
		List<connectionId_t> result = new ArrayList<connectionId_t>();
		
		if (connectionIds == null){
			return result;
		}
		
		for (int i=0; i<connectionIds.size(); i++){
			result.add(getConnectionIdType(connectionIds.get(i)));
		}
		
		return result;
	}
	
	
	
	private connectionId_t getConnectionIdType(ConnectionId connectionId){
		if (connectionId == null){
			return null;
		}
		
		connectionId_t result = FlowMessage.connectionId_t.newBuilder().
									setDestinationCEPId((int)connectionId.getDestinationCEPId()).
									setQosId(connectionId.getQosId()).
									setSourceCEPId((int)connectionId.getSourceCEPId()).
									build();
		
		return result;
	}
}
