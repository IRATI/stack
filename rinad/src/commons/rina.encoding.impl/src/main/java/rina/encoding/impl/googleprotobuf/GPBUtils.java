package rina.encoding.impl.googleprotobuf;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.concurrent.ConcurrentHashMap;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import rina.aux.Property;
import rina.encoding.impl.googleprotobuf.apnaminginfo.ApplicationProcessNamingInfoMessage;
import rina.encoding.impl.googleprotobuf.apnaminginfo.ApplicationProcessNamingInfoMessage.applicationProcessNamingInfo_t;
import rina.encoding.impl.googleprotobuf.qosspecification.QoSSpecification;
import rina.encoding.impl.googleprotobuf.property.PropertyMessage;
import rina.encoding.impl.googleprotobuf.property.PropertyMessage.property_t;
import rina.encoding.impl.googleprotobuf.qosspecification.QoSSpecification.qosSpecification_t;

import com.google.protobuf.ByteString;

import eu.irati.librina.ApplicationProcessNamingInformation;
import eu.irati.librina.FlowSpecification;

/**
 * Utility classes to work with Google Protocol Buffers
 * @author eduardgrasa
 *
 */
public class GPBUtils {
	
	private static final Log log = LogFactory.getLog(GPBUtils.class);
	
	public static ApplicationProcessNamingInformation getApplicationProcessNamingInfo(applicationProcessNamingInfo_t apNamingInfo) {
		String apName = GPBUtils.getString(apNamingInfo.getApplicationProcessName());
		String apInstance = GPBUtils.getString(apNamingInfo.getApplicationProcessInstance());
		String aeName = GPBUtils.getString(apNamingInfo.getApplicationEntityName());
		String aeInstance = GPBUtils.getString(apNamingInfo.getApplicationEntityInstance());
		
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
	
	public static property_t getPropertyT(Property property){
		if (property != null){
			return PropertyMessage.property_t.newBuilder().
					setName(property.getName()).
					setValue(property.getValue()).
				    build();
		}else{
			return null;
		}
	}
	
	private static property_t getPropertyT(Entry<String, String> entry){
		return PropertyMessage.property_t.newBuilder().
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
