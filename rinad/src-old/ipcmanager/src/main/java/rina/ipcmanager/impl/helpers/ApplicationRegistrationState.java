package rina.ipcmanager.impl.helpers;

import eu.irati.librina.ApplicationProcessNamingInformation;

import java.util.ArrayList;
import java.util.List;

/**
 * Stores the state of an application registration
 * @author eduardgrasa
 *
 */
public class ApplicationRegistrationState {

	/** The name of the application */
	private ApplicationProcessNamingInformation applicationName;
	
	/** The list of DIFs where the application is registered */
	private List<String> difNames;
	
	public ApplicationRegistrationState(ApplicationProcessNamingInformation applicationName){
		this.applicationName = applicationName;
		difNames = new ArrayList<String>();
	}
	
	public ApplicationProcessNamingInformation getApplicationName(){
		return applicationName;
	}
	
	public List<String> getDIFNames(){
		return difNames;
	}

}
