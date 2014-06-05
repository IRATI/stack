package rina.encoding.impl.googleprotobuf;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.concurrent.ConcurrentHashMap;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import rina.utils.Property;
import rina.encoding.impl.googleprotobuf.apnaminginfo.ApplicationProcessNamingInfoMessage;
import rina.encoding.impl.googleprotobuf.apnaminginfo.ApplicationProcessNamingInfoMessage.applicationProcessNamingInfo_t;
import rina.encoding.impl.googleprotobuf.common.CommonMessages;
import rina.encoding.impl.googleprotobuf.common.CommonMessages.property_t;
import rina.encoding.impl.googleprotobuf.connectionpolicies.ConnectionPoliciesMessage;
import rina.encoding.impl.googleprotobuf.connectionpolicies.ConnectionPoliciesMessage.connectionPolicies_t;
import rina.encoding.impl.googleprotobuf.connectionpolicies.ConnectionPoliciesMessage.dtcpConfig_t;
import rina.encoding.impl.googleprotobuf.connectionpolicies.ConnectionPoliciesMessage.dtcpFlowControlConfig_t;
import rina.encoding.impl.googleprotobuf.connectionpolicies.ConnectionPoliciesMessage.dtcpRateBasedFlowControlConfig_t;
import rina.encoding.impl.googleprotobuf.connectionpolicies.ConnectionPoliciesMessage.dtcpRtxControlConfig_t;
import rina.encoding.impl.googleprotobuf.connectionpolicies.ConnectionPoliciesMessage.dtcpWindowBasedFlowControlConfig_t;
import rina.encoding.impl.googleprotobuf.policy.PolicyDescriptorMessage;
import rina.encoding.impl.googleprotobuf.policy.PolicyDescriptorMessage.policyDescriptor_t;
import rina.encoding.impl.googleprotobuf.qosspecification.QoSSpecification;
import rina.encoding.impl.googleprotobuf.qosspecification.QoSSpecification.qosSpecification_t;

import com.google.protobuf.ByteString;

import eu.irati.librina.ApplicationProcessNamingInformation;
import eu.irati.librina.ConnectionPolicies;
import eu.irati.librina.DTCPConfig;
import eu.irati.librina.DTCPFlowControlConfig;
import eu.irati.librina.DTCPRateBasedFlowControlConfig;
import eu.irati.librina.DTCPRtxControlConfig;
import eu.irati.librina.DTCPWindowBasedFlowControlConfig;
import eu.irati.librina.FlowSpecification;
import eu.irati.librina.PolicyConfig;
import eu.irati.librina.PolicyParameter;
import eu.irati.librina.PolicyParameterList;

/**
 * Utility classes to work with Google Protocol Buffers
 * @author eduardgrasa
 *
 */
public class GPBUtils {
	
	private static final Log log = LogFactory.getLog(GPBUtils.class);
	
	public static ApplicationProcessNamingInformation getApplicationProcessNamingInfo(applicationProcessNamingInfo_t apNamingInfo) {
		String apName = apNamingInfo.getApplicationProcessName();
		String apInstance = apNamingInfo.getApplicationProcessInstance();
		String aeName = apNamingInfo.getApplicationEntityName();
		String aeInstance = apNamingInfo.getApplicationEntityInstance();
		
		ApplicationProcessNamingInformation result = 
				new ApplicationProcessNamingInformation(apName, apInstance);
		result.setEntityName(aeName);
		result.setEntityInstance(aeInstance);
		return result;
	}
	
	public static applicationProcessNamingInfo_t getApplicationProcessNamingInfoT(
			ApplicationProcessNamingInformation apNamingInfo){
		if (apNamingInfo != null){
			String apName = GPBUtils.getGPBString(apNamingInfo.getProcessName());
			String apInstance = GPBUtils.getGPBString(apNamingInfo.getProcessInstance());
			String aeName = GPBUtils.getGPBString(apNamingInfo.getEntityName());
			String aeInstance = GPBUtils.getGPBString(apNamingInfo.getEntityInstance());
			return ApplicationProcessNamingInfoMessage.applicationProcessNamingInfo_t.newBuilder().
			setApplicationProcessName(apName).
			setApplicationProcessInstance(apInstance).
			setApplicationEntityName(aeName).
			setApplicationEntityInstance(aeInstance).
			build();
		}else{
			return null;
		}
	}
	
	public static policyDescriptor_t getPolicyDescriptorType(PolicyConfig policyConfig) {
		policyDescriptor_t result = PolicyDescriptorMessage.policyDescriptor_t.getDefaultInstance();
		
		if (policyConfig == null) {
			return result;
		}
				
		result = PolicyDescriptorMessage.policyDescriptor_t.newBuilder().
				setPolicyImplName(policyConfig.getName()).
				setVersion(policyConfig.getVersion()).
				build();
		
		return result;
	}
	
	private static List<property_t> getAllPolicyParameters(PolicyParameterList list) {
		List<property_t> result = new ArrayList<property_t>();
		if (list == null || list.isEmpty()) {
			return result;
		}
		
		Iterator<PolicyParameter> iterator = list.iterator();
		while (iterator.hasNext()) {
			result.add(getPropertyType(iterator.next()));
		}
		
		return result;
	}
	
	private static property_t getPropertyType(PolicyParameter parameter) {
		property_t result = null;
		if (parameter == null){
			return CommonMessages.property_t.getDefaultInstance();
		}
		
		result = CommonMessages.property_t.newBuilder().
				setName(parameter.getName()).
				setValue(parameter.getValue()).
				build();
		
		return result;
	}
	
	public static property_t getPropertyT(Property property){
		if (property != null){
			return CommonMessages.property_t.newBuilder().
					setName(property.getName()).
					setValue(property.getValue()).
				    build();
		}else{
			return null;
		}
	}
	
	private static property_t getPropertyT(Entry<String, String> entry){
		return CommonMessages.property_t.newBuilder().
			setName(entry.getKey()).
			setValue(entry.getValue()).
			build();
	}
	
	public static List<property_t> getProperties(Map<String, String> properties){
		List<property_t> result = new ArrayList<property_t>();
		if (properties == null){
			return result;
		}
		
		Iterator<Entry<String, String>> iterator = properties.entrySet().iterator();
		while(iterator.hasNext()){
			result.add(getPropertyT(iterator.next()));
		}
		
		return result;
	}
	
	public static Property getProperty(property_t property_t){
		if (property_t == null){
			return null;
		}
		
		Property property = new Property();
		property.setName(property_t.getName());
		property.setValue(property_t.getValue());
		
		return property;
	}
	
	public static PolicyConfig getPolicyConfig(policyDescriptor_t policyConfig) {
		PolicyConfig result = null;
		
		if (policyConfig == null || policyConfig.getPolicyImplName() == null) {
			return result;
		}
		
		if (policyConfig.getPolicyImplName() != null && 
				policyConfig.getPolicyImplName().equals("default")) {
			return result;
		}
		
		result = new PolicyConfig(policyConfig.getPolicyImplName(), policyConfig.getVersion());
		
		return result;
	}
	
	private static PolicyParameter getPolicyParameter(property_t parameter) {
		if (parameter == null){
			return new PolicyParameter();
		}
		
		return new PolicyParameter(parameter.getName(), parameter.getValue());
	}
	
	public static Map<String, String> getProperties(List<property_t> gpbProperties){
		if (gpbProperties == null){
			return null;
		}
		
		Map<String, String> result = new ConcurrentHashMap<String, String>();
		log.debug("Got "+gpbProperties.size()+" properties");
		for(int i=0; i<gpbProperties.size(); i++){
			log.debug("Name: "+ gpbProperties.get(i).getName()+ "; Value: "+ gpbProperties.get(i).getValue());
			result.put(gpbProperties.get(i).getName(), gpbProperties.get(i).getValue());
		}
		
		return result;
	}
	
	public static ConnectionPolicies getConnectionPolicies(connectionPolicies_t connectionPolicies){
		ConnectionPolicies result = new ConnectionPolicies();
		PolicyConfig policyConfig = null;
		
		result.setDtcpPresent(connectionPolicies.getDtcpPresent());
		result.setInitialATimer(connectionPolicies.getInitialATimer());
		policyConfig = GPBUtils.getPolicyConfig(connectionPolicies.getInitialseqnumpolicy());
		if (policyConfig != null) {
			result.setInitialseqnumpolicy(policyConfig);
		}
		result.setSeqnumrolloverthreshold((int)connectionPolicies.getSeqnumrolloverthreshold());
		if (result.isDtcpPresent()) {
			result.setDtcpConfiguration(getDTCPConfig(connectionPolicies.getDtcpConfiguration()));
		}
		
		return result;
	}
	
	private static DTCPConfig getDTCPConfig(dtcpConfig_t dtcpConfig) {
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
		
		policyConfig = GPBUtils.getPolicyConfig(dtcpConfig.getRttestimatorpolicy());
		if (policyConfig != null) {
			result.setRttestimatorpolicy(policyConfig);
		}
		
		return result;
	}
	
	private static DTCPRtxControlConfig getDTCPRtxControlConfig(dtcpRtxControlConfig_t rtxControlConfig) {
		DTCPRtxControlConfig result = new DTCPRtxControlConfig();
		PolicyConfig policyConfig = null;
		
		if (rtxControlConfig == null) {
			return result;
		}
		
		result.setDatarxmsnmax(rtxControlConfig.getDatarxmsnmax());
		
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
	 
	private static DTCPFlowControlConfig getDTCPFlowControlConfig(dtcpFlowControlConfig_t flowControlConfig) {
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
		
		policyConfig = GPBUtils.getPolicyConfig(flowControlConfig.getReceivingflowcontrolpolicy());
		if (policyConfig != null) {
			result.setReceivingflowcontrolpolicy(policyConfig);
		}
		
		return result;
	}
	
	private static DTCPRateBasedFlowControlConfig getDTCPRateBasedFlowControlConfig(
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
	
	private static DTCPWindowBasedFlowControlConfig getDTCPWindowBasedFlowControlConfig(
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
		policyConfig = GPBUtils.getPolicyConfig(flowControlConfig.getTxcontrolpolicy());
		if (policyConfig != null) {
			result.setTxControlPolicy(policyConfig);
		}
		
		return result;
	}
	
	public static FlowSpecification getFlowSpecification(qosSpecification_t qosSpec_t){
		if (qosSpec_t == null){
			return null;
		}
		
		FlowSpecification result = new FlowSpecification();
		result.setAverageBandwidth(qosSpec_t.getAverageBandwidth());
		result.setAverageSduBandwidth(qosSpec_t.getAverageSDUBandwidth());
		result.setDelay(qosSpec_t.getDelay());
		result.setJitter(qosSpec_t.getJitter());
		result.setMaxAllowableGap(qosSpec_t.getMaxAllowableGapSdu());
		result.setOrderedDelivery(qosSpec_t.getOrder());
		result.setPartialDelivery(qosSpec_t.getPartialDelivery());
		result.setPeakBandwidthDuration(qosSpec_t.getPeakBandwidthDuration());
		result.setPeakSduBandwidthDuration(qosSpec_t.getPeakSDUBandwidthDuration());
		result.setUndetectedBitErrorRate(qosSpec_t.getUndetectedBitErrorRate());
		
		return result;
	}
	
	public static qosSpecification_t getQoSSpecificationT(FlowSpecification flowSpec){
		if (flowSpec != null){
			
			return QoSSpecification.qosSpecification_t.newBuilder().
				setName(GPBUtils.getGPBString("")).
				setQosid(0).
				setAverageBandwidth(flowSpec.getAverageBandwidth()).
				setAverageSDUBandwidth(flowSpec.getAverageSduBandwidth()).
				setDelay((int)flowSpec.getDelay()).
				setJitter((int)flowSpec.getJitter()).
				setMaxAllowableGapSdu(flowSpec.getMaxAllowableGap()).
				setOrder(flowSpec.isOrderedDelivery()).
				setPartialDelivery(flowSpec.isPartialDelivery()).
				setPeakBandwidthDuration((int)flowSpec.getPeakBandwidthDuration()).
				setPeakSDUBandwidthDuration((int)flowSpec.getPeakSduBandwidthDuration()).
				setUndetectedBitErrorRate(flowSpec.getUndetectedBitErrorRate()).
				build();
		}else{
			return null;
		}
	}
	
	public static connectionPolicies_t getConnectionPoliciesType(ConnectionPolicies connectionPolicies) {
		if (connectionPolicies == null) {
			return null;
		}
		
		connectionPolicies_t result = null;
		
		dtcpConfig_t dtcpConfig = null;
		if (connectionPolicies.isDtcpPresent()) {
			dtcpConfig = getDTCPConfigType(connectionPolicies.getDtcpConfiguration());
		} else {
			dtcpConfig = ConnectionPoliciesMessage.dtcpConfig_t.getDefaultInstance();
		}

		result = ConnectionPoliciesMessage.connectionPolicies_t.newBuilder().
				setDtcpPresent(connectionPolicies.isDtcpPresent()).
				setDtcpConfiguration(dtcpConfig).
				setInitialATimer(connectionPolicies.getInitialATimer()).
				setInitialseqnumpolicy(GPBUtils.getPolicyDescriptorType(connectionPolicies.getInitialseqnumpolicy())).
				setSeqnumrolloverthreshold(connectionPolicies.getSeqnumrolloverthreshold()).
				build();

		return result;
	}
	
	private static dtcpConfig_t getDTCPConfigType(DTCPConfig dtcpConfig) {
		if (dtcpConfig == null) {
			return ConnectionPoliciesMessage.dtcpConfig_t.getDefaultInstance();
		}
		
		dtcpRtxControlConfig_t rtxConfig = null;
		if (dtcpConfig.isRtxcontrol()) {
			rtxConfig = getDTCPRtxControlConfigType(dtcpConfig.getRtxcontrolconfig());
		} else {
			rtxConfig = ConnectionPoliciesMessage.dtcpRtxControlConfig_t.getDefaultInstance();
		}
		
		dtcpFlowControlConfig_t flowConfig = null;
		if (dtcpConfig.isFlowcontrol()) {
			flowConfig = getDTCPFlowControlConfigType(dtcpConfig.getFlowcontrolconfig());
		} else {
			flowConfig = ConnectionPoliciesMessage.dtcpFlowControlConfig_t.getDefaultInstance();
		}
		
		return ConnectionPoliciesMessage.dtcpConfig_t.newBuilder().
				setFlowControl(dtcpConfig.isFlowcontrol()).
				setFlowControlConfig(flowConfig).
				setRtxControl(dtcpConfig.isRtxcontrol()).
				setRtxControlConfig(rtxConfig).
				setInitialrecvrinactivitytime(dtcpConfig.getInitialrecvrinactivitytime()).
				setInitialsenderinactivitytime(dtcpConfig.getInitialsenderinactivitytime()).
				setLostcontrolpdupolicy(GPBUtils.getPolicyDescriptorType(dtcpConfig.getLostcontrolpdupolicy())).
				setSendertimerinactiviypolicy(GPBUtils.getPolicyDescriptorType(dtcpConfig.getSendertimerinactiviypolicy())).
				setRcvrtimerinactivitypolicy(GPBUtils.getPolicyDescriptorType(dtcpConfig.getRcvrtimerinactivitypolicy())).
				setRttestimatorpolicy(GPBUtils.getPolicyDescriptorType(dtcpConfig.getRttestimatorpolicy())).
				build();
	}
	
	private static dtcpRtxControlConfig_t getDTCPRtxControlConfigType(DTCPRtxControlConfig rtxControl) {
		if (rtxControl == null) {
			return ConnectionPoliciesMessage.dtcpRtxControlConfig_t.getDefaultInstance();
		}
		
		return ConnectionPoliciesMessage.dtcpRtxControlConfig_t.newBuilder().
				setDatarxmsnmax(rtxControl.getDatarxmsnmax()).
				setRcvrackpolicy(GPBUtils.getPolicyDescriptorType(rtxControl.getRcvrackpolicy())).
				setRcvrcontrolackpolicy(GPBUtils.getPolicyDescriptorType(rtxControl.getRcvrcontrolackpolicy())).
				setRecvingacklistpolicy(GPBUtils.getPolicyDescriptorType(rtxControl.getRecvingacklistpolicy())).
				setRtxtimerexpirypolicy(GPBUtils.getPolicyDescriptorType(rtxControl.getRtxtimerexpirypolicy())).
				setSenderackpolicy(GPBUtils.getPolicyDescriptorType(rtxControl.getSenderackpolicy())).
				setSendingackpolicy(GPBUtils.getPolicyDescriptorType(rtxControl.getSendingackpolicy())).build();
	}
	
	private static dtcpFlowControlConfig_t getDTCPFlowControlConfigType(DTCPFlowControlConfig flowControl) {
		if (flowControl == null) {
			return ConnectionPoliciesMessage.dtcpFlowControlConfig_t.getDefaultInstance();
		}
		
		dtcpWindowBasedFlowControlConfig_t windowConfig = null;
		if (flowControl.isWindowbased()) {
			windowConfig = getDTCPWindowBasedFlowControlType(flowControl.getWindowbasedconfig());
		} else {
			windowConfig = ConnectionPoliciesMessage.dtcpWindowBasedFlowControlConfig_t.getDefaultInstance();
		}
		
		dtcpRateBasedFlowControlConfig_t rateConfig = null;
		if (flowControl.isRatebased()) {
			rateConfig = getDTCPRateBasedFlowControlType(flowControl.getRatebasedconfig());
		} else {
			rateConfig = ConnectionPoliciesMessage.dtcpRateBasedFlowControlConfig_t.getDefaultInstance();
		}
		
		return ConnectionPoliciesMessage.dtcpFlowControlConfig_t.newBuilder().
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
				setReceivingflowcontrolpolicy(GPBUtils.getPolicyDescriptorType(flowControl.getReceivingflowcontrolpolicy())).
				build();
	}
	
	private static dtcpRateBasedFlowControlConfig_t getDTCPRateBasedFlowControlType(
			DTCPRateBasedFlowControlConfig rate) {
		if (rate == null) {
			return ConnectionPoliciesMessage.dtcpRateBasedFlowControlConfig_t.getDefaultInstance();
		}
		
		return ConnectionPoliciesMessage.dtcpRateBasedFlowControlConfig_t.newBuilder().
				setSendingrate(rate.getSendingrate()).
				setTimeperiod(rate.getTimeperiod()).
				setNooverridedefaultpeakpolicy(GPBUtils.getPolicyDescriptorType(rate.getNooverridedefaultpeakpolicy())).
				setNorateslowdownpolicy(GPBUtils.getPolicyDescriptorType(rate.getNorateslowdownpolicy())).
				setRatereductionpolicy(GPBUtils.getPolicyDescriptorType(rate.getRatereductionpolicy())).
				build();
	}
	
	private static dtcpWindowBasedFlowControlConfig_t getDTCPWindowBasedFlowControlType(
			DTCPWindowBasedFlowControlConfig window) {
		if (window == null) {
			return ConnectionPoliciesMessage.dtcpWindowBasedFlowControlConfig_t.getDefaultInstance();
		}
		
		return ConnectionPoliciesMessage.dtcpWindowBasedFlowControlConfig_t.newBuilder().
				setInitialcredit(window.getInitialcredit()).
				setMaxclosedwindowqueuelength(window.getMaxclosedwindowqueuelength()).
				setRcvrflowcontrolpolicy(GPBUtils.getPolicyDescriptorType(window.getRcvrflowcontrolpolicy())).
				setTxcontrolpolicy(GPBUtils.getPolicyDescriptorType(window.getTxControlPolicy())).
				build();
	}
	
	public static byte[] getByteArray(ByteString byteString){
		byte[] result = null;
		if (!byteString.equals(ByteString.EMPTY)){
			result = byteString.toByteArray();
		}
		
		return result;
	}
	
	public static String getString(String string){
		String result = null;
		if (!string.equals("")){
			result = string;
		}
		
		return result;
	}
	
	public static ByteString getByteString(byte[] byteArray){
		ByteString result = ByteString.EMPTY;
		if (byteArray != null){
			result = ByteString.copyFrom(byteArray);
		}
		
		return result;
	}
	
	public static String getGPBString(String string){
		String result = string;
		if (result == null){
			result = "";
		}
		
		return result;
	}

}
