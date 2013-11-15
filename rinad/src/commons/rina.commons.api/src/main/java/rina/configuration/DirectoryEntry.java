package rina.configuration;

public class DirectoryEntry {

	private String applicationProcessName = null;
	private String applicationProcessInstance = null;
	private String applicationEntityName = null;
	private String hostname = null;
	private int socketPortNumber = -1;
	
	public String getApplicationProcessName() {
		return applicationProcessName;
	}
	public void setApplicationProcessName(String applicationProcessName) {
		this.applicationProcessName = applicationProcessName;
	}
	public String getApplicationProcessInstance() {
		return applicationProcessInstance;
	}
	public void setApplicationProcessInstance(String applicationProcessInstance) {
		this.applicationProcessInstance = applicationProcessInstance;
	}
	public String getApplicationEntityName() {
		return applicationEntityName;
	}
	public void setApplicationEntityName(String applicationEntityName) {
		this.applicationEntityName = applicationEntityName;
	}
	public String getHostname() {
		return hostname;
	}
	public void setHostname(String hostname) {
		this.hostname = hostname;
	}
	public int getSocketPortNumber() {
		return socketPortNumber;
	}
	public void setSocketPortNumber(int socketPortNumber) {
		this.socketPortNumber = socketPortNumber;
	}
}
