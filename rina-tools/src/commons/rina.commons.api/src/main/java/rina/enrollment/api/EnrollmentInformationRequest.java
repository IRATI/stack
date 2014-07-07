package rina.enrollment.api;

import java.util.List;

import eu.irati.librina.ApplicationProcessNamingInformation;

import rina.ribdaemon.api.RIBObjectNames;

/**
 * The object that contains all the information 
 * that is required to initiate an enrollment 
 * request (send as the objectvalue of a CDAP M_START 
 * message, as specified by the Enrollment spec)
 * @author eduardgrasa
 *
 */
public class EnrollmentInformationRequest {
	
	public static final String ENROLLMENT_INFO_OBJECT_NAME = RIBObjectNames.SEPARATOR + RIBObjectNames.DAF + 
		RIBObjectNames.SEPARATOR + RIBObjectNames.MANAGEMENT + RIBObjectNames.SEPARATOR + RIBObjectNames.ENROLLMENT;
	public static final String ENROLLMENT_INFO_OBJECT_CLASS = "enrollment information";
	
	/**
	 * The address of the IPC Process that requests 
	 * to join a DIF
	 */
	private long address = 0L;
	
	private List<ApplicationProcessNamingInformation> supportingDifs;

	public long getAddress() {
		return address;
	}

	public void setAddress(long address) {
		this.address = address;
	}

	public List<ApplicationProcessNamingInformation> getSupportingDifs() {
		return supportingDifs;
	}

	public void setSupportingDifs(
			List<ApplicationProcessNamingInformation> supportingDifs) {
		this.supportingDifs = supportingDifs;
	}
}
