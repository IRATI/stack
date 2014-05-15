package rina.encoding.impl.googleprotobuf.flow;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

import eu.irati.librina.Connection;
import eu.irati.librina.ConnectionPolicies;
import eu.irati.librina.DTCPConfig;
import eu.irati.librina.DTCPFlowControlConfig;
import eu.irati.librina.DTCPRateBasedFlowControlConfig;
import eu.irati.librina.DTCPRtxControlConfig;
import eu.irati.librina.DTCPWindowBasedFlowControlConfig;
import eu.irati.librina.EFCPPolicyConfig;
import eu.irati.librina.FlowSpecification;
import eu.irati.librina.PolicyParameter;
import eu.irati.librina.PolicyParameterList;

import rina.encoding.api.Encoder;
import rina.encoding.impl.googleprotobuf.GPBUtils;
import rina.encoding.impl.googleprotobuf.flow.FlowMessage.connectionId_t;
import rina.encoding.impl.googleprotobuf.flow.FlowMessage.connectionPolicies_t;
import rina.encoding.impl.googleprotobuf.flow.FlowMessage.dtcpConfig_t;
import rina.encoding.impl.googleprotobuf.flow.FlowMessage.dtcpFlowControlConfig_t;
import rina.encoding.impl.googleprotobuf.flow.FlowMessage.dtcpRateBasedFlowControlConfig_t;
import rina.encoding.impl.googleprotobuf.flow.FlowMessage.dtcpRtxControlConfig_t;
import rina.encoding.impl.googleprotobuf.flow.FlowMessage.dtcpWindowBasedFlowControlConfig_t;
import rina.encoding.impl.googleprotobuf.flow.FlowMessage.efcpPolicyConfig_t;
import rina.encoding.impl.googleprotobuf.flow.FlowMessage.policyParameter_t;
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
		
		result.setDtcPpresent(connectionPolicies.getDtcpPresent());
		result.setSeqnumrolloverthreshold((int)connectionPolicies.getSeqnumrolloverthreshold());
		if (result.isDtcPpresent()) {
			result.setDtcpConfiguration(getDTCPConfig(connectionPolicies.getDtcpConfiguration()));
		}
		
		return result;
	}
	
	private DTCPConfig getDTCPConfig(dtcpConfig_t dtcpConfig) {
		DTCPConfig result = new DTCPConfig();
		EFCPPolicyConfig policyConfig = null;
		
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
		policyConfig = getEFCPPolicyConfig(dtcpConfig.getInitialseqnumpolicy());
		if (policyConfig != null) {
			result.setInitialseqnumpolicy(policyConfig);
		}
		policyConfig = getEFCPPolicyConfig(dtcpConfig.getLostcontrolpdupolicy());
		if (policyConfig != null) {
			result.setLostcontrolpdupolicy(policyConfig);
		}
		policyConfig = getEFCPPolicyConfig(dtcpConfig.getRcvrtimerinactivitypolicy());
		if (policyConfig != null) {
			result.setRcvrtimerinactivitypolicy(policyConfig);
		}
		policyConfig = getEFCPPolicyConfig(dtcpConfig.getSendertimerinactiviypolicy());
		if (policyConfig != null) {
			result.setSendertimerinactiviypolicy(policyConfig);
		}
		
		return result;
	}
	
	private DTCPRtxControlConfig getDTCPRtxControlConfig(dtcpRtxControlConfig_t rtxControlConfig) {
		DTCPRtxControlConfig result = new DTCPRtxControlConfig();
		EFCPPolicyConfig policyConfig = null;
		
		if (rtxControlConfig == null) {
			return result;
		}
		
		result.setDatarxmsnmax(rtxControlConfig.getDatarxmsnmax());
		result.setInitialATimer(rtxControlConfig.getInitialATimer());
		policyConfig = getEFCPPolicyConfig(rtxControlConfig.getRcvrackpolicy());
		if (policyConfig != null) {
			result.setRcvrackpolicy(policyConfig);
		}
		policyConfig = getEFCPPolicyConfig(rtxControlConfig.getRcvrcontrolackpolicy());
		if (policyConfig != null) {
			result.setRcvrcontrolackpolicy(policyConfig);
		}
		policyConfig = getEFCPPolicyConfig(rtxControlConfig.getRecvingacklistpolicy());
		if (policyConfig != null) {
			result.setRecvingacklistpolicy(policyConfig);
		}
		policyConfig = getEFCPPolicyConfig(rtxControlConfig.getRttestimatorpolicy());
		if (policyConfig != null) {
			result.setRttestimatorpolicy(policyConfig);
		}
		policyConfig = getEFCPPolicyConfig(rtxControlConfig.getRtxtimerexpirypolicy());
		if (policyConfig != null) {
			result.setRtxtimerexpirypolicy(policyConfig);
		}
		policyConfig = getEFCPPolicyConfig(rtxControlConfig.getSenderackpolicy());
		if (policyConfig != null) {
			result.setSenderackpolicy(policyConfig);
		}
		policyConfig = getEFCPPolicyConfig(rtxControlConfig.getSendingackpolicy());
		if (policyConfig != null) {
			result.setSendingackpolicy(policyConfig);
		}
		
		return result;
	}
	 
	private DTCPFlowControlConfig getDTCPFlowControlConfig(dtcpFlowControlConfig_t flowControlConfig) {
		DTCPFlowControlConfig result = new DTCPFlowControlConfig();
		EFCPPolicyConfig policyConfig = null;
		
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
		policyConfig = getEFCPPolicyConfig(flowControlConfig.getClosedwindowpolicy());
		if (policyConfig != null) {
			result.setClosedwindowpolicy(policyConfig);
		}
		policyConfig = getEFCPPolicyConfig(flowControlConfig.getFlowcontroloverrunpolicy());
		if (policyConfig != null) {
			result.setFlowcontroloverrunpolicy(policyConfig);
		}
		policyConfig = getEFCPPolicyConfig(flowControlConfig.getReconcileflowcontrolpolicy());
		if (policyConfig != null) {
			result.setReconcileflowcontrolpolicy(policyConfig);
		}
		
		return result;
	}
	
	private DTCPRateBasedFlowControlConfig getDTCPRateBasedFlowControlConfig(
			dtcpRateBasedFlowControlConfig_t flowControlConfig) {
		DTCPRateBasedFlowControlConfig result = new DTCPRateBasedFlowControlConfig();
		EFCPPolicyConfig policyConfig = null;
		
		if (flowControlConfig == null) {
			return result;
		}
		
		result.setSendingrate((int)flowControlConfig.getSendingrate());
		result.setTimeperiod((int)flowControlConfig.getTimeperiod());
		policyConfig = getEFCPPolicyConfig(flowControlConfig.getNooverridedefaultpeakpolicy());
		if (policyConfig != null) {
			result.setNooverridedefaultpeakpolicy(policyConfig);
		}
		policyConfig = getEFCPPolicyConfig(flowControlConfig.getNorateslowdownpolicy());
		if (policyConfig != null) {
			result.setNorateslowdownpolicy(policyConfig);
		}
		policyConfig = getEFCPPolicyConfig(flowControlConfig.getRatereductionpolicy());
		if (policyConfig != null) {
			result.setRatereductionpolicy(policyConfig);
		}
		
		return result;
	}
	
	private DTCPWindowBasedFlowControlConfig getDTCPWindowBasedFlowControlConfig(
			dtcpWindowBasedFlowControlConfig_t flowControlConfig) {
		DTCPWindowBasedFlowControlConfig result = new DTCPWindowBasedFlowControlConfig();
		EFCPPolicyConfig policyConfig = null;
		
		if (flowControlConfig == null) {
			return result;
		}
		
		result.setInitialcredit((int)flowControlConfig.getInitialcredit());
		result.setMaxclosedwindowqueuelength((int)flowControlConfig.getMaxclosedwindowqueuelength());
		policyConfig = getEFCPPolicyConfig(flowControlConfig.getRcvrflowcontrolpolicy());
		if (policyConfig != null) {
			result.setRcvrflowcontrolpolicy(policyConfig);
		}
		policyConfig = getEFCPPolicyConfig(flowControlConfig.getReceivingflowcontrolpolicy());
		if (policyConfig != null) {
			result.setReceivingflowcontrolpolicy(policyConfig);
		}
		
		return result;
	}
	
	private EFCPPolicyConfig getEFCPPolicyConfig(efcpPolicyConfig_t policyConfig) {
		EFCPPolicyConfig result = null;
		
		if (policyConfig == null || policyConfig.getName() == null) {
			return result;
		}
		
		if (policyConfig.getName() != null && 
				policyConfig.getName().equals("default")) {
			return result;
		}
		
		result = new EFCPPolicyConfig();
		result.setName(policyConfig.getName());
		result.setVersion((short)policyConfig.getVersion());
		if (policyConfig.getParametersCount() > 0) {
			List<policyParameter_t> parameters = policyConfig.getParametersList();
			for(int i=0; i<parameters.size(); i++){
				result.addParameter(getPolicyParameter(parameters.get(i)));
			}
		}
		
		return result;
	}
	
	private PolicyParameter getPolicyParameter(policyParameter_t parameter) {
		PolicyParameter result = new PolicyParameter();
		if (parameter == null){
			return result;
		}
		
		result.setName(parameter.getName());
		result.setValue(parameter.getValue());
		
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
				setInitialseqnumpolicy(getEFCPPolicyConfigType(dtcpConfig.getInitialseqnumpolicy())).
				setLostcontrolpdupolicy(getEFCPPolicyConfigType(dtcpConfig.getLostcontrolpdupolicy())).
				setSendertimerinactiviypolicy(getEFCPPolicyConfigType(dtcpConfig.getSendertimerinactiviypolicy())).
				setRcvrtimerinactivitypolicy(getEFCPPolicyConfigType(dtcpConfig.getRcvrtimerinactivitypolicy())).
				build();
	}
	
	private dtcpRtxControlConfig_t getDTCPRtxControlConfigType(DTCPRtxControlConfig rtxControl) {
		if (rtxControl == null) {
			return FlowMessage.dtcpRtxControlConfig_t.getDefaultInstance();
		}
		
		return FlowMessage.dtcpRtxControlConfig_t.newBuilder().
				setDatarxmsnmax(rtxControl.getDatarxmsnmax()).
				setInitialATimer(rtxControl.getInitialATimer()).
				setRcvrackpolicy(getEFCPPolicyConfigType(rtxControl.getRcvrackpolicy())).
				setRcvrcontrolackpolicy(getEFCPPolicyConfigType(rtxControl.getRcvrcontrolackpolicy())).
				setRecvingacklistpolicy(getEFCPPolicyConfigType(rtxControl.getRecvingacklistpolicy())).
				setRttestimatorpolicy(getEFCPPolicyConfigType(rtxControl.getRttestimatorpolicy())).
				setRtxtimerexpirypolicy(getEFCPPolicyConfigType(rtxControl.getRtxtimerexpirypolicy())).
				setSenderackpolicy(getEFCPPolicyConfigType(rtxControl.getSenderackpolicy())).
				setSendingackpolicy(getEFCPPolicyConfigType(rtxControl.getSendingackpolicy())).build();
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
				setClosedwindowpolicy(getEFCPPolicyConfigType(flowControl.getClosedwindowpolicy())).
				setFlowcontroloverrunpolicy(getEFCPPolicyConfigType(flowControl.getFlowcontroloverrunpolicy())).
				setReconcileflowcontrolpolicy(getEFCPPolicyConfigType(flowControl.getReconcileflowcontrolpolicy())).
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
				setNooverridedefaultpeakpolicy(getEFCPPolicyConfigType(rate.getNooverridedefaultpeakpolicy())).
				setNorateslowdownpolicy(getEFCPPolicyConfigType(rate.getNorateslowdownpolicy())).
				setRatereductionpolicy(getEFCPPolicyConfigType(rate.getRatereductionpolicy())).
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
				setRcvrflowcontrolpolicy(getEFCPPolicyConfigType(window.getRcvrflowcontrolpolicy())).
				setReceivingflowcontrolpolicy(getEFCPPolicyConfigType(window.getReceivingflowcontrolpolicy())).
				build();
	}
	
	private efcpPolicyConfig_t getEFCPPolicyConfigType(EFCPPolicyConfig policyConfig) {
		efcpPolicyConfig_t result = FlowMessage.efcpPolicyConfig_t.getDefaultInstance();
		
		if (policyConfig == null) {
			return result;
		}
		
		List<policyParameter_t> parameters = getAllPolicyParameters(policyConfig.getParameters());
				
		result = FlowMessage.efcpPolicyConfig_t.newBuilder().
				setName(policyConfig.getName()).
				setVersion(policyConfig.getVersion()).
				addAllParameters(parameters).
				build();
		
		return result;
	}
	
	List<policyParameter_t> getAllPolicyParameters(PolicyParameterList list) {
		List<policyParameter_t> result = new ArrayList<policyParameter_t>();
		if (list == null || list.isEmpty()) {
			return result;
		}
		
		Iterator<PolicyParameter> iterator = list.iterator();
		while (iterator.hasNext()) {
			result.add(getPolicyParameterType(iterator.next()));
		}
		
		return result;
	}
	
	policyParameter_t getPolicyParameterType(PolicyParameter parameter) {
		policyParameter_t result = null;
		if (parameter == null){
			return FlowMessage.policyParameter_t.getDefaultInstance();
		}
		
		result = FlowMessage.policyParameter_t.newBuilder().
				setName(parameter.getName()).
				setValue(parameter.getValue()).
				build();
		
		return result;
	}
}
