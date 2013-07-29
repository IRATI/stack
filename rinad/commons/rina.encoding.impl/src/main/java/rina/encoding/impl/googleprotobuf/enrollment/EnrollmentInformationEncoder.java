package rina.encoding.impl.googleprotobuf.enrollment;

import rina.encoding.api.BaseEncoder;
import rina.encoding.impl.googleprotobuf.enrollment.EnrollmentInformationMessage.enrollmentInformation_t;
import rina.enrollment.api.EnrollmentInformationRequest;

public class EnrollmentInformationEncoder extends BaseEncoder{

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

		return eiRequest;
	}

	public synchronized byte[] encode(Object object) throws Exception {
		if (object == null || !(object instanceof EnrollmentInformationRequest)){
			throw new Exception("This is not the encoder for objects of type " + EnrollmentInformationRequest.class.toString());
		}
		
		return convertModelToGPB((EnrollmentInformationRequest)object).toByteArray();
	}
	
	public static enrollmentInformation_t convertModelToGPB(EnrollmentInformationRequest enrollmentInformation){
		enrollmentInformation_t gpbEnrollmentInfo = EnrollmentInformationMessage.enrollmentInformation_t.newBuilder().
			setAddress(enrollmentInformation.getAddress()).
			build();
		
		return gpbEnrollmentInfo;
	}

}
