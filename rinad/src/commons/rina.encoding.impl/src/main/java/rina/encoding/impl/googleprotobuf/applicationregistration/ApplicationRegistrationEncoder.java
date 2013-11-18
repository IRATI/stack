package rina.encoding.impl.googleprotobuf.applicationregistration;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

import eu.irati.librina.ApplicationProcessNamingInformation;
import eu.irati.librina.ApplicationProcessNamingInformationList;
import eu.irati.librina.ApplicationRegistration;

import rina.encoding.api.Encoder;
import rina.encoding.impl.googleprotobuf.GPBUtils;

public class ApplicationRegistrationEncoder implements Encoder{
	
	public synchronized Object decode(byte[] serializedObject, Class<?> objectClass) throws Exception {
		if (objectClass == null || !(objectClass.equals(ApplicationRegistration.class))){
			throw new Exception("This is not the serializer for objects of type "+objectClass.getName());
		}
		
		ApplicationRegistrationMessage.ApplicationRegistration gpbApplicationRegistration = ApplicationRegistrationMessage.ApplicationRegistration.parseFrom(serializedObject);
		
		ApplicationProcessNamingInformation apName = GPBUtils.
				getApplicationProcessNamingInfo(gpbApplicationRegistration.getNamingInfo());
		
		ApplicationRegistration result = new ApplicationRegistration(apName);
		List<String> difs = gpbApplicationRegistration.getDifNamesList();
		ApplicationProcessNamingInformation aux = null;
		for(int i=0; i<difs.size(); i++) {
			aux = new ApplicationProcessNamingInformation();
			aux.setProcessName(difs.get(i));
			result.addDIFName(aux);
		}
		
		return result;
	}
	
	public synchronized byte[] encode(Object object) throws Exception {
		if (object == null || !(object instanceof ApplicationRegistration)){
			throw new Exception("This is not the serializer for objects of type "+ApplicationRegistration.class.toString());
		}

		ApplicationRegistration applicationRegistration = (ApplicationRegistration) object;
		List<String> difNames = getDIFNamesAsStrings(applicationRegistration.getDIFNames());
		
		ApplicationRegistrationMessage.ApplicationRegistration gpbApplicationRegistration = 
			ApplicationRegistrationMessage.ApplicationRegistration.newBuilder().
						setNamingInfo(GPBUtils.getApplicationProcessNamingInfoT(applicationRegistration.getApplicationName())).
						setSocketNumber(0).
						addAllDifNames(difNames).
						build();

		return gpbApplicationRegistration.toByteArray();
	}
	
	private List<String> getDIFNamesAsStrings(ApplicationProcessNamingInformationList list) {
		List<String> result = new ArrayList<String>();
		Iterator<ApplicationProcessNamingInformation> iterator = list.iterator();
		while (iterator.hasNext()) {
			result.add(iterator.next().getProcessName());
		}
		
		return result;
	}
}