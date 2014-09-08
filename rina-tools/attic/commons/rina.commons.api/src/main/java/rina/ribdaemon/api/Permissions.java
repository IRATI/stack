package rina.ribdaemon.api;

import java.util.List;

import eu.irati.librina.ApplicationProcessNamingInformation;

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
	private List<ApplicationProcessNamingInformation> applications = null;
	
	/**
	 * Information of the CDAP operations that are allowed 
	 * at this node
	 */
	private CDAPOperationsPermission cdapOperationPermissions = null;
	
	public Permissions(List<ApplicationProcessNamingInformation> applications, 
			CDAPOperationsPermission cdapOperationPermissions){
		this.applications = applications;
		this.cdapOperationPermissions = cdapOperationPermissions;
	}

	public List<ApplicationProcessNamingInformation> getApplications() {
		return applications;
	}

	public void setApplications(List<ApplicationProcessNamingInformation> applications) {
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
