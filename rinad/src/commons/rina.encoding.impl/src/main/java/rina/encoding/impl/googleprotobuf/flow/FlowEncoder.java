package rina.encoding.impl.googleprotobuf.flow;

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
import eu.irati.librina.FlowSpecification;

import rina.encoding.api.Encoder;
import rina.encoding.impl.googleprotobuf.GPBUtils;
import rina.encoding.impl.googleprotobuf.flow.FlowMessage.connectionId_t;
import rina.encoding.impl.googleprotobuf.flow.FlowMessage.connectionPolicies_t;
import rina.encoding.impl.googleprotobuf.flow.FlowMessage.dtcpConfig_t;
import rina.encoding.impl.googleprotobuf.flow.FlowMessage.dtcpFlowControlConfig_t;
import rina.encoding.impl.googleprotobuf.flow.FlowMessage.dtcpRateBasedFlowControlConfig_t;
import rina.encoding.impl.googleprotobuf.flow.FlowMessage.dtcpRtxControlConfig_t;
import rina.encoding.impl.googleprotobuf.flow.FlowMessage.dtcpWindowBasedFlowControlConfig_t;
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
		flow.setConnectionPolicies(getConnectionPolicies(gpbFlow.getConnectionPolicies()));
		qosSpecification_t qosParams = gpbFlow.getQosParameters();
		if (!qosParams.equals(qosSpecification_t.getDefaultInstance())){
			flow.setFlowSpecification(GPBUtils.getFlowSpecification(qosParams));
		} else {
			flow.setFlowSpecification(new FlowSpecification());
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
	
	private ConnectionPolicies getConnectionPolicies(connectionPolicies_t connectionPolicies){
		ConnectionPolicies result = new ConnectionPolicies();
		PolicyConfig policyConfig = null;
		
		result.setDtcPpresent(connectionPolicies.getDtcpPresent());
		policyConfig = GPBUtils.getPolicyConfig(connectionPolicies.getInitialseqnumpolicy());
		if (policyConfig != null) {
			result.setInitialseqnumpolicy(policyConfig);
		}
		result.setSeqnumrolloverthreshold((int)connectionPolicies.getSeqnumrolloverthreshold());
		if (result.isDtcPpresent()) {
			result.setDtcpConfiguration(getDTCPConfig(connectionPolicies.getDtcpConfiguration()));
		}
		
		return result;
	}
	
	private DTCPConfig getDTCPConfig(dtcpConfig_t dtcpConfig) {
		DTCPConfig result = new DTCPConfig();
		PolicyConfig policyConfig = null;
		
		if (dtcpConfig == null){
			return result;
		}
		
		result.setFlowcontrol(dtcpConfig.getFlowControl());
		if (result.isFlowcontrol()) {
			result.setFlowcontrolconfig(getDTCPFlowControlConfig(dtcpConfig.getFlowControlConfig()));
		}
		result.setRtxcontrol(dtcpConfig.getRtxControl());
		if (result.isRtxcontrol()) {
			result.setRtxcontrolconfig(getDTCPRtxControlConfig(dtcpConfig.getRtxControlConfig()));
		}
		result.setInitialrecvrinactivitytime(dtcpConfig.getInitialrecvrinactivitytime());
		result.setInitialsenderinactivitytime(dtcpConfig.getInitialsenderinactivitytime());
		policyConfig = GPBUtils.getPolicyConfig(dtcpConfig.getLostcontrolpdupolicy());
		if (policyConfig != null) {
			result.setLostcontrolpdupolicy(policyConfig);
		}
		policyConfig = GPBUtils.getPolicyConfig(dtcpConfig.getRcvrtimerinactivitypolicy());
		if (policyConfig != null) {
			result.setRcvrtimerinactivitypolicy(policyConfig);
		}
		policyConfig = GPBUtils.getPolicyConfig(dtcpConfig.getSendertimerinactiviypolicy());
		if (policyConfig != null) {
			result.setSendertimerinactiviypolicy(policyConfig);
		}
		
		return result;
	}
	
	private DTCPRtxControlConfig getDTCPRtxControlConfig(dtcpRtxControlConfig_t rtxControlConfig) {
		DTCPRtxControlConfig result = new DTCPRtxControlConfig();
		PolicyConfig policyConfig = null;
		
		if (rtxControlConfig == null) {
			return result;
		}
		
		result.setDatarxmsnmax(rtxControlConfig.getDatarxmsnmax());
		result.setInitialATimer(rtxControlConfig.getInitialATimer());
		policyConfig = GPBUtils.getPolicyConfig(rtxControlConfig.getRcvrackpolicy());
		if (policyConfig != null) {
			result.setRcvrackpolicy(policyConfig);
		}
		policyConfig = GPBUtils.getPolicyConfig(rtxControlConfig.getRcvrcontrolackpolicy());
		if (policyConfig != null) {
			result.setRcvrcontrolackpolicy(policyConfig);
		}
		policyConfig = GPBUtils.getPolicyConfig(rtxControlConfig.getRecvingacklistpolicy());
		if (policyConfig != null) {
			result.setRecvingacklistpolicy(policyConfig);
		}
		policyConfig = GPBUtils.getPolicyConfig(rtxControlConfig.getRttestimatorpolicy());
		if (policyConfig != null) {
			result.setRttestimatorpolicy(policyConfig);
		}
		policyConfig = GPBUtils.getPolicyConfig(rtxControlConfig.getRtxtimerexpirypolicy());
		if (policyConfig != null) {
			result.setRtxtimerexpirypolicy(policyConfig);
		}
		policyConfig = GPBUtils.getPolicyConfig(rtxControlConfig.getSenderackpolicy());
		if (policyConfig != null) {
			result.setSenderackpolicy(policyConfig);
		}
		policyConfig = GPBUtils.getPolicyConfig(rtxControlConfig.getSendingackpolicy());
		if (policyConfig != null) {
			result.setSendingackpolicy(policyConfig);
		}
		
		return result;
	}
	 
	private DTCPFlowControlConfig getDTCPFlowControlConfig(dtcpFlowControlConfig_t flowControlConfig) {
		DTCPFlowControlConfig result = new DTCPFlowControlConfig();
		PolicyConfig policyConfig = null;
		
		if (flowControlConfig == null) {
			return result;
		}
		
		result.setRatebased(flowControlConfig.getRateBased());
		if (result.isRatebased()){
			result.setRatebasedconfig(getDTCPRateBasedFlowControlConfig(flowControlConfig.getRateBasedConfig()));
		}
		result.setWindowbased(flowControlConfig.getWindowBased());
		if (result.isWindowbased()){
			result.setWindowbasedconfig(getDTCPWindowBasedFlowControlConfig(flowControlConfig.getWindowBasedConfig()));
		}
		result.setRcvbuffersthreshold((int)flowControlConfig.getRcvbuffersthreshold());
		result.setRcvbytespercentthreshold((int)flowControlConfig.getRcvbytespercentthreshold());
		result.setRcvbytesthreshold((int)flowControlConfig.getRcvbytesthreshold());
		result.setSentbuffersthreshold((int)flowControlConfig.getSentbuffersthreshold());
		result.setSentbytespercentthreshold((int)flowControlConfig.getSentbytespercentthreshold());
		result.setSentbytesthreshold((int)flowControlConfig.getSentbytesthreshold());
		policyConfig = GPBUtils.getPolicyConfig(flowControlConfig.getClosedwindowpolicy());
		if (policyConfig != null) {
			result.setClosedwindowpolicy(policyConfig);
		}
		policyConfig = GPBUtils.getPolicyConfig(flowControlConfig.getFlowcontroloverrunpolicy());
		if (policyConfig != null) {
			result.setFlowcontroloverrunpolicy(policyConfig);
		}
		policyConfig = GPBUtils.getPolicyConfig(flowControlConfig.getReconcileflowcontrolpolicy());
		if (policyConfig != null) {
			result.setReconcileflowcontrolpolicy(policyConfig);
		}
		
		return result;
	}
	
	private DTCPRateBasedFlowControlConfig getDTCPRateBasedFlowControlConfig(
			dtcpRateBasedFlowControlConfig_t flowControlConfig) {
		DTCPRateBasedFlowControlConfig result = new DTCPRateBasedFlowControlConfig();
		PolicyConfig policyConfig = null;
		
		if (flowControlConfig == null) {
			return result;
		}
		
		result.setSendingrate((int)flowControlConfig.getSendingrate());
		result.setTimeperiod((int)flowControlConfig.getTimeperiod());
		policyConfig = GPBUtils.getPolicyConfig(flowControlConfig.getNooverridedefaultpeakpolicy());
		if (policyConfig != null) {
			result.setNooverridedefaultpeakpolicy(policyConfig);
		}
		policyConfig = GPBUtils.getPolicyConfig(flowControlConfig.getNorateslowdownpolicy());
		if (policyConfig != null) {
			result.setNorateslowdownpolicy(policyConfig);
		}
		policyConfig = GPBUtils.getPolicyConfig(flowControlConfig.getRatereductionpolicy());
		if (policyConfig != null) {
			result.setRatereductionpolicy(policyConfig);
		}
		
		return result;
	}
	
	private DTCPWindowBasedFlowControlConfig getDTCPWindowBasedFlowControlConfig(
			dtcpWindowBasedFlowControlConfig_t flowControlConfig) {
		DTCPWindowBasedFlowControlConfig result = new DTCPWindowBasedFlowControlConfig();
		PolicyConfig policyConfig = null;
		
		if (flowControlConfig == null) {
			return result;
		}
		
		result.setInitialcredit((int)flowControlConfig.getInitialcredit());
		result.setMaxclosedwindowqueuelength((int)flowControlConfig.getMaxclosedwindowqueuelength());
		policyConfig = GPBUtils.getPolicyConfig(flowControlConfig.getRcvrflowcontrolpolicy());
		if (policyConfig != null) {
			result.setRcvrflowcontrolpolicy(policyConfig);
		}
		policyConfig = GPBUtils.getPolicyConfig(flowControlConfig.getReceivingflowcontrolpolicy());
		if (policyConfig != null) {
			result.setReceivingflowcontrolpolicy(policyConfig);
		}
		
		return result;
	}
	
	public synchronized byte[] encode(Object object) throws Exception {
		if (object == null || !(object instanceof Flow)){
			throw new Exception("This is not the serializer for objects of type "+Flow.class.toString());
		}
		
		Flow flow = (Flow) object;
		qosSpecification_t qosSpecificationT = GPBUtils.getQoSSpecificationT(flow.getFlowSpecification());
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
			setConnectionPolicies(getConnectionPoliciesType(flow.getConnectionPolicies())).
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
	
	private connectionPolicies_t getConnectionPoliciesType(ConnectionPolicies connectionPolicies) {
		if (connectionPolicies == null) {
			return null;
		}
		
		connectionPolicies_t result = null;
		
		dtcpConfig_t dtcpConfig = null;
		if (connectionPolicies.isDtcPpresent()) {
			dtcpConfig = getDTCPConfigType(connectionPolicies.getDtcpConfiguration());
		} else {
			dtcpConfig = FlowMessage.dtcpConfig_t.getDefaultInstance();
		}

		result = FlowMessage.connectionPolicies_t.newBuilder().
				setDtcpPresent(connectionPolicies.isDtcPpresent()).
				setDtcpConfiguration(dtcpConfig).
				setInitialseqnumpolicy(GPBUtils.getPolicyDescriptorType(connectionPolicies.getInitialseqnumpolicy())).
				setSeqnumrolloverthreshold(connectionPolicies.getSeqnumrolloverthreshold()).
				build();

		return result;
	}
	
	private dtcpConfig_t getDTCPConfigType(DTCPConfig dtcpConfig) {
		if (dtcpConfig == null) {
			return FlowMessage.dtcpConfig_t.getDefaultInstance();
		}
		
		dtcpRtxControlConfig_t rtxConfig = null;
		if (dtcpConfig.isRtxcontrol()) {
			rtxConfig = getDTCPRtxControlConfigType(dtcpConfig.getRtxcontrolconfig());
		} else {
			rtxConfig = FlowMessage.dtcpRtxControlConfig_t.getDefaultInstance();
		}
		
		dtcpFlowControlConfig_t flowConfig = null;
		if (dtcpConfig.isFlowcontrol()) {
			flowConfig = getDTCPFlowControlConfigType(dtcpConfig.getFlowcontrolconfig());
		} else {
			flowConfig = FlowMessage.dtcpFlowControlConfig_t.getDefaultInstance();
		}
		
		return FlowMessage.dtcpConfig_t.newBuilder().
				setFlowControl(dtcpConfig.isFlowcontrol()).
				setFlowControlConfig(flowConfig).
				setRtxControl(dtcpConfig.isRtxcontrol()).
				setRtxControlConfig(rtxConfig).
				setInitialrecvrinactivitytime(dtcpConfig.getInitialrecvrinactivitytime()).
				setInitialsenderinactivitytime(dtcpConfig.getInitialsenderinactivitytime()).
				setLostcontrolpdupolicy(GPBUtils.getPolicyDescriptorType(dtcpConfig.getLostcontrolpdupolicy())).
				setSendertimerinactiviypolicy(GPBUtils.getPolicyDescriptorType(dtcpConfig.getSendertimerinactiviypolicy())).
				setRcvrtimerinactivitypolicy(GPBUtils.getPolicyDescriptorType(dtcpConfig.getRcvrtimerinactivitypolicy())).
				build();
	}
	
	private dtcpRtxControlConfig_t getDTCPRtxControlConfigType(DTCPRtxControlConfig rtxControl) {
		if (rtxControl == null) {
			return FlowMessage.dtcpRtxControlConfig_t.getDefaultInstance();
		}
		
		return FlowMessage.dtcpRtxControlConfig_t.newBuilder().
				setDatarxmsnmax(rtxControl.getDatarxmsnmax()).
				setInitialATimer(rtxControl.getInitialATimer()).
				setRcvrackpolicy(GPBUtils.getPolicyDescriptorType(rtxControl.getRcvrackpolicy())).
				setRcvrcontrolackpolicy(GPBUtils.getPolicyDescriptorType(rtxControl.getRcvrcontrolackpolicy())).
				setRecvingacklistpolicy(GPBUtils.getPolicyDescriptorType(rtxControl.getRecvingacklistpolicy())).
				setRttestimatorpolicy(GPBUtils.getPolicyDescriptorType(rtxControl.getRttestimatorpolicy())).
				setRtxtimerexpirypolicy(GPBUtils.getPolicyDescriptorType(rtxControl.getRtxtimerexpirypolicy())).
				setSenderackpolicy(GPBUtils.getPolicyDescriptorType(rtxControl.getSenderackpolicy())).
				setSendingackpolicy(GPBUtils.getPolicyDescriptorType(rtxControl.getSendingackpolicy())).build();
	}
	
	private dtcpFlowControlConfig_t getDTCPFlowControlConfigType(DTCPFlowControlConfig flowControl) {
		if (flowControl == null) {
			return FlowMessage.dtcpFlowControlConfig_t.getDefaultInstance();
		}
		
		dtcpWindowBasedFlowControlConfig_t windowConfig = null;
		if (flowControl.isWindowbased()) {
			windowConfig = getDTCPWindowBasedFlowControlType(flowControl.getWindowbasedconfig());
		} else {
			windowConfig = FlowMessage.dtcpWindowBasedFlowControlConfig_t.getDefaultInstance();
		}
		
		dtcpRateBasedFlowControlConfig_t rateConfig = null;
		if (flowControl.isRatebased()) {
			rateConfig = getDTCPRateBasedFlowControlType(flowControl.getRatebasedconfig());
		} else {
			rateConfig = FlowMessage.dtcpRateBasedFlowControlConfig_t.getDefaultInstance();
		}
		
		return FlowMessage.dtcpFlowControlConfig_t.newBuilder().
				setWindowBased(flowControl.isWindowbased()).
				setWindowBasedConfig(windowConfig).
				setRateBased(flowControl.isRatebased()).
				setRateBasedConfig(rateConfig).setRcvbuffersthreshold(flowControl.getRcvbuffersthreshold()).
				setRcvbytespercentthreshold(flowControl.getRcvbytespercentthreshold()).
				setRcvbytesthreshold(flowControl.getRcvbytesthreshold()).
				setSentbuffersthreshold(flowControl.getSentbuffersthreshold()).
				setSentbytespercentthreshold(flowControl.getSentbytespercentthreshold()).
				setSentbytesthreshold(flowControl.getSentbytesthreshold()).
				setClosedwindowpolicy(GPBUtils.getPolicyDescriptorType(flowControl.getClosedwindowpolicy())).
				setFlowcontroloverrunpolicy(GPBUtils.getPolicyDescriptorType(flowControl.getFlowcontroloverrunpolicy())).
				setReconcileflowcontrolpolicy(GPBUtils.getPolicyDescriptorType(flowControl.getReconcileflowcontrolpolicy())).
				build();
	}
	
	private dtcpRateBasedFlowControlConfig_t getDTCPRateBasedFlowControlType(
			DTCPRateBasedFlowControlConfig rate) {
		if (rate == null) {
			return FlowMessage.dtcpRateBasedFlowControlConfig_t.getDefaultInstance();
		}
		
		return FlowMessage.dtcpRateBasedFlowControlConfig_t.newBuilder().
				setSendingrate(rate.getSendingrate()).
				setTimeperiod(rate.getTimeperiod()).
				setNooverridedefaultpeakpolicy(GPBUtils.getPolicyDescriptorType(rate.getNooverridedefaultpeakpolicy())).
				setNorateslowdownpolicy(GPBUtils.getPolicyDescriptorType(rate.getNorateslowdownpolicy())).
				setRatereductionpolicy(GPBUtils.getPolicyDescriptorType(rate.getRatereductionpolicy())).
				build();
	}
	
	private dtcpWindowBasedFlowControlConfig_t getDTCPWindowBasedFlowControlType(
			DTCPWindowBasedFlowControlConfig window) {
		if (window == null) {
			return FlowMessage.dtcpWindowBasedFlowControlConfig_t.getDefaultInstance();
		}
		
		return FlowMessage.dtcpWindowBasedFlowControlConfig_t.newBuilder().
				setInitialcredit(window.getInitialcredit()).
				setMaxclosedwindowqueuelength(window.getMaxclosedwindowqueuelength()).
				setRcvrflowcontrolpolicy(GPBUtils.getPolicyDescriptorType(window.getRcvrflowcontrolpolicy())).
				setReceivingflowcontrolpolicy(GPBUtils.getPolicyDescriptorType(window.getReceivingflowcontrolpolicy())).
				build();
	}
}
