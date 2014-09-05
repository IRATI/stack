package rina.encoding.impl.googleprotobuf.enrollment;

import java.util.ArrayList;
import java.util.List;

import eu.irati.librina.ApplicationProcessNamingInformation;

import rina.encoding.api.Encoder;
import rina.encoding.impl.googleprotobuf.enrollment.EnrollmentInformationMessage.enrollmentInformation_t;
import rina.enrollment.api.EnrollmentInformationRequest;

public class EnrollmentInformationEncoder implements Encoder{

	public synchronized Object decode(byte[] encodedObject, Class<?> objectClass) throws Exception {
		if (objectClass == null || !(objectClass.equals(EnrollmentInformationRequest.class))){
			throw new Exception("This is not the encoder for objects of type "+objectClass.getName());
		}
		
		enrollmentInformation_t gpbEnrollmentInfo = EnrollmentInformationMessage.enrollmentInformation_t.parseFrom(encodedObject);
		return convertGPBToModel(gpbEnrollmentInfo);
	}
	
	public static EnrollmentInformationRequest convertGPBToModel(enrollmentInformation_t gpbEnrollmentInfo){
		EnrollmentInformationRequest eiRequest = new EnrollmentInformationRequest();
		eiRequest.setAddress(gpbEnrollmentInfo.getAddress());
		List<String> supportingDifs = gpbEnrollmentInfo.getSupportingDifsList();
		if (supportingDifs != null) {
			List<ApplicationProcessNamingInformation> list = new ArrayList<ApplicationProcessNamingInformation>();
			for(int i=0; i<supportingDifs.size(); i++) {
				list.add(new ApplicationProcessNamingInformation(supportingDifs.get(i), ""));
			}
			
			eiRequest.setSupportingDifs(list);
		}
		
		return eiRequest;
	}

	public synchronized byte[] encode(Object object) throws Exception {
		if (object == null || !(object instanceof EnrollmentInformationRequest)){
			throw new Exception("This is not the encoder for objects of type " + EnrollmentInformationRequest.class.toString());
		}
		
		return convertModelToGPB((EnrollmentInformationRequest)object).toByteArray();
	}
	
	public static enrollmentInformation_t convertModelToGPB(EnrollmentInformationRequest enrollmentInformation){
		List<String> supportingDifs = new ArrayList<String>();
		if (enrollmentInformation.getSupportingDifs() != null) {
			for(int i=0; i<enrollmentInformation.getSupportingDifs().size(); i++){
				supportingDifs.add(enrollmentInformation.getSupportingDifs().get(i).getProcessName());
			}
		}
		enrollmentInformation_t gpbEnrollmentInfo = EnrollmentInformationMessage.enrollmentInformation_t.newBuilder().
			setAddress(enrollmentInformation.getAddress()).
			addAllSupportingDifs(supportingDifs).
			build();
		
		return gpbEnrollmentInfo;
	}

}
