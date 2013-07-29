package rina.enrollment.api;

import java.util.List;

import rina.applicationprocess.api.ApplicationProcessNamingInfo;
import rina.cdap.api.CDAPSessionDescriptor;
import rina.cdap.api.message.CDAPMessage;
import rina.ipcprocess.api.IPCProcessComponent;
import rina.ribdaemon.api.RIBObjectNames;

/**
 * The enrollment task manages the members of the DIF. It implements the state machines that are used 
 * to join a DIF or to collaboarate with a remote IPC Process to allow him to join the DIF.
 * @author eduardgrasa
 *
 */
public interface EnrollmentTask extends IPCProcessComponent{
	
	public static final String ENROLLMENT_RIB_OBJECT_NAME = RIBObjectNames.SEPARATOR + RIBObjectNames.DAF + 
		RIBObjectNames.SEPARATOR + RIBObjectNames.MANAGEMENT + RIBObjectNames.SEPARATOR + RIBObjectNames.ENROLLMENT;

	public static final String ENROLLMENT_RIB_OBJECT_CLASS = "enrollment information";	
	
	/**
	 * A remote IPC process Connect request has been received
	 * @param cdapMessage
	 * @param cdapSessionDescriptor
	 */
	public void connect(CDAPMessage cdapMessage, CDAPSessionDescriptor cdapSessionDescriptor);
	
	/**
	 * A remote IPC process Connect response has been received
	 * @param cdapMessage
	 * @param cdapSessionDescriptor
	 */
	public void connectResponse(CDAPMessage cdapMessage, CDAPSessionDescriptor cdapSessionDescriptor);
	
	/**
	 * A remote IPC process Release request has been received
	 * @param cdapMessage
	 * @param cdapSessionDescriptor
	 */
	public void release(CDAPMessage cdapMessage, CDAPSessionDescriptor cdapSessionDescriptor);
	
	/**
	 * A remote IPC process Release response has been received
	 * @param cdapMessage
	 * @param cdapSessionDescriptor
	 */
	public void releaseResponse(CDAPMessage cdapMessage, CDAPSessionDescriptor cdapSessionDescriptor);
	
	/**
	 * Starts the enrollment program
	 * @param cdapMessage
	 * @param cdapSessionDescriptor
	 */
	public void initiateEnrollment(Neighbor candidate);
	
	 /**
	 * Called by the enrollment state machine when the enrollment request has been completed, either successfully or unsuccessfully
	 * @param candidate the IPC process we were trying to enroll to
	 * @param enrollee true if this IPC process is the one that initiated the 
	 * enrollment sequence (i.e. it is the application process that wants to 
	 * join the DIF)
	 */
	public void enrollmentCompleted(Neighbor candidate, boolean enrollee);
	
	/**
	 * Called by the enrollment state machine when the enrollment sequence fails
	 * @param remotePeer
	 * @param portId
	 * @param enrollee
	 * @param sendMessage
	 * @param reason
	 */
	 public void enrollmentFailed(ApplicationProcessNamingInfo remotePeerNamingInfo, int portId, String reason, boolean enrolle, boolean sendReleaseMessage);
	
	/**
	 * Finds out if the ICP process is already enrolled to the IPC process identified by 
	 * the provided apNamingInfo
	 * @param apNamingInfo
	 * @return
	 */
	public boolean isEnrolledTo(String applicationProcessName);
	
	/**
	 * Return the list of IPC Process names we're currently enrolled to
	 * @return
	 */
	public List<String> getEnrolledIPCProcessNames();
}
