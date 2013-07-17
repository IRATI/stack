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
import rina.applicationprocess.api.ApplicationProcessNamingInfo;
import rina.ipcservice.api.QualityOfServiceSpecification;

import com.google.protobuf.ByteString;

/**
 * Utility classes to work with Google Protocol Buffers
 * @author eduardgrasa
 *
 */
public class GPBUtils {
	
	private static final Log log = LogFactory.getLog(GPBUtils.class);
	
	public static ApplicationProcessNamingInfo getApplicationProcessNamingInfo(applicationProcessNamingInfo_t apNamingInfo) {
		String apName = GPBUtils.getString(apNamingInfo.getApplicationProcessName());
		String apInstance = GPBUtils.getString(apNamingInfo.getApplicationProcessInstance());
		String aeName = GPBUtils.getString(apNamingInfo.getApplicationEntityName());
		String aeInstance = GPBUtils.getString(apNamingInfo.getApplicationEntityInstance());
		
		ApplicationProcessNamingInfo result = new ApplicationProcessNamingInfo(apName, apInstance);
		result.setApplicationEntityName(aeName);
		result.setApplicationEntityInstance(aeInstance);
		return result;
	}
	
	public static applicationProcessNamingInfo_t getApplicationProcessNamingInfoT(ApplicationProcessNamingInfo apNamingInfo){
		if (apNamingInfo != null){
			String apName = GPBUtils.getGPBString(apNamingInfo.getApplicationProcessName());
			String apInstance = GPBUtils.getGPBString(apNamingInfo.getApplicationProcessInstance());
			String aeName = GPBUtils.getGPBString(apNamingInfo.getApplicationEntityName());
			String aeInstance = GPBUtils.getGPBString(apNamingInfo.getApplicationEntityInstance());
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
	
	public static QualityOfServiceSpecification getQualityOfServiceSpecification(qosSpecification_t qosSpec_t){
		if (qosSpec_t == null){
			return null;
		}
		
		QualityOfServiceSpecification result = new QualityOfServiceSpecification();
		result.setName(GPBUtils.getString(qosSpec_t.getName()));
		result.setQosCubeId(qosSpec_t.getQosid());
		result.setAverageBandwidth(qosSpec_t.getAverageBandwidth());
		result.setAverageSDUBandwidth(qosSpec_t.getAverageSDUBandwidth());
		result.setDelay(qosSpec_t.getDelay());
		result.setJitter(qosSpec_t.getJitter());
		result.setMaxAllowableGapSDU(qosSpec_t.getMaxAllowableGapSdu());
		result.setOrder(qosSpec_t.getOrder());
		result.setPartialDelivery(qosSpec_t.getPartialDelivery());
		result.setPeakBandwidthDuration(qosSpec_t.getPeakBandwidthDuration());
		result.setPeakSDUBandwidthDuration(qosSpec_t.getPeakSDUBandwidthDuration());
		result.setUndetectedBitErrorRate(qosSpec_t.getUndetectedBitErrorRate());
		
		for(int i=0; i<qosSpec_t.getExtraParametersCount(); i++){
			result.getExtendedPrameters().put(
					qosSpec_t.getExtraParametersList().get(i).getName(), qosSpec_t.getExtraParametersList().get(i).getValue());
		}
		
		return result;
	}
	
	public static qosSpecification_t getQoSSpecificationT(QualityOfServiceSpecification qualityOfServiceSpecification){
		if (qualityOfServiceSpecification != null){
			List<property_t> extraParameters = getQosSpecExtraParametersType(qualityOfServiceSpecification);
			
			return QoSSpecification.qosSpecification_t.newBuilder().
				setName(GPBUtils.getGPBString(qualityOfServiceSpecification.getName())).
				setQosid(qualityOfServiceSpecification.getQosCubeId()).
				setAverageBandwidth(qualityOfServiceSpecification.getAverageBandwidth()).
				setAverageSDUBandwidth(qualityOfServiceSpecification.getAverageSDUBandwidth()).
				setDelay(qualityOfServiceSpecification.getDelay()).
				addAllExtraParameters(extraParameters).
				setJitter(qualityOfServiceSpecification.getJitter()).
				setMaxAllowableGapSdu(qualityOfServiceSpecification.getMaxAllowableGapSDU()).
				setOrder(qualityOfServiceSpecification.isOrder()).
				setPartialDelivery(qualityOfServiceSpecification.isPartialDelivery()).
				setPeakBandwidthDuration(qualityOfServiceSpecification.getPeakBandwidthDuration()).
				setPeakSDUBandwidthDuration(qualityOfServiceSpecification.getPeakSDUBandwidthDuration()).
				setUndetectedBitErrorRate(qualityOfServiceSpecification.getUndetectedBitErrorRate()).
				build();
		}else{
			return null;
		}
	}
	
	private static List<property_t> getQosSpecExtraParametersType(QualityOfServiceSpecification qualityOfServiceSpecification){
		List<property_t> qosParametersList = new ArrayList<property_t>();
		
		if (qualityOfServiceSpecification.getExtendedPrameters().isEmpty()){
			return qosParametersList;
		}
		
		Iterator<Entry<String, String>> iterator = qualityOfServiceSpecification.getExtendedPrameters().entrySet().iterator();
		Entry<String, String> entry = null;
		while(iterator.hasNext()){
			entry = iterator.next();
			qosParametersList.add(getPropertyT(entry));
		}
		
		return qosParametersList;
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
