package rina.ribdaemon.api;

import java.util.List;

import rina.applicationprocess.api.ApplicationProcessNamingInfo;

/**
 * Permissions of a certain application on 
 * a RIB node
 * @author eduardgrasa
 *
 */
public class Permissions {
	/**
	 * All the applications that have these permissions
	 */
	private List<ApplicationProcessNamingInfo> applications = null;
	
	/**
	 * Information of the CDAP operations that are allowed 
	 * at this node
	 */
	private CDAPOperationsPermission cdapOperationPermissions = null;
	
	public Permissions(List<ApplicationProcessNamingInfo> applications, 
			CDAPOperationsPermission cdapOperationPermissions){
		this.applications = applications;
		this.cdapOperationPermissions = cdapOperationPermissions;
	}

	public List<ApplicationProcessNamingInfo> getApplications() {
		return applications;
	}

	public void setApplications(List<ApplicationProcessNamingInfo> applications) {
		this.applications = applications;
	}

	public CDAPOperationsPermission getCdapOperationPermissions() {
		return cdapOperationPermissions;
	}

	public void setCdapOperationPermissions(
			CDAPOperationsPermission cdapOperationPermissions) {
		this.cdapOperationPermissions = cdapOperationPermissions;
	}
}
