package rina.enrollment.api;

import rina.applicationprocess.api.ApplicationProcessNamingInfo;

/**
 * Creates, destroys and stores instances of the Enrollment Task. Each 
 * instance of a Enrollment Task maps to an instance of an IPC Process 
 * @author eduardgrasa
 *
 */
public interface EnrollmentTaskFactory {
	
	/**
	 * Creates a new Enrollment Task
	 * TODO add more stuff probably
	 * @param namingInfo the name of the IPC process where this Enrollment Task belongs
	 */
	public EnrollmentTask createEnrollmentTask(ApplicationProcessNamingInfo namingInfo);
	
	/**
	 * Destroys an existing Enrollment Task
	 * TODO add more stuff probably
	 * @param namingInfo the name of the IPC process where this Enrollment Task belongs
	 */
	public void destroyEnrollmentTask(ApplicationProcessNamingInfo namingInfo);
	
	/**
	 * Get an existing Enrollment Task
	 * TODO add more stuff probably
	 * @param namingInfo the name of the IPC process where Enrollment Task belongs
	 */
	public EnrollmentTask getEnrollmentTask(ApplicationProcessNamingInfo namingInfo);
}
