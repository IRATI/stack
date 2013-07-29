package rina.ipcservice.api;

import java.util.List;

import rina.applicationprocess.api.ApplicationProcessNamingInfo;

/**
 * Issued by the RINA library to the IPC Manager when an application wants to express that it
 * is willing to accept flows.
 * @author eduardgrasa
 *
 */
public class ApplicationRegistration {
	
	public static final String OBJECT_NAME="/applicationregistration";
	public static final String OBJECT_CLASS="applicationregistration";
	
	private ApplicationProcessNamingInfo apNamingInfo = null;
	
	private int socketNumber = 0;
	
	/**
	 * The list of DIFs that this application wants to register. If the list is
	 * null it means all DIFs available in that system.
	 */
	private List<String> difNames = null;

	public ApplicationProcessNamingInfo getApNamingInfo() {
		return apNamingInfo;
	}

	public void setApNamingInfo(ApplicationProcessNamingInfo apNamingInfo) {
		this.apNamingInfo = apNamingInfo;
	}

	public void setSocketNumber(int socketNumber) {
		this.socketNumber = socketNumber;
	}

	public int getSocketNumber() {
		return socketNumber;
	}

	public void setDifNames(List<String> difNames) {
		this.difNames = difNames;
	}

	public List<String> getDifNames() {
		return difNames;
	}

}
