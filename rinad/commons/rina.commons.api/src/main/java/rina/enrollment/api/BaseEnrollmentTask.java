package rina.enrollment.api;

import rina.ipcprocess.api.BaseIPCProcessComponent;

/**
 * Provides the component name for the Enrollment Task
 * @author eduardgrasa
 *
 */
public abstract class BaseEnrollmentTask extends BaseIPCProcessComponent implements EnrollmentTask{
	
	public static final String getComponentName(){
		return EnrollmentTask.class.getName();
	}

	public String getName(){
		return BaseEnrollmentTask.getComponentName();
	}
}
