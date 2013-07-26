package rina.configuration;

public class ExpectedApplicationRegistration {
	
	private String applicationProcessName = null;
	private String applicationProcessInstance = null;
	private String applicationEntityName = null;
	
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
	public int getSocketPortNumber() {
		return socketPortNumber;
	}
	public void setSocketPortNumber(int socketPortNumber) {
		this.socketPortNumber = socketPortNumber;
	}
	public String getApplicationEntityName() {
		return applicationEntityName;
	}
	public void setApplicationEntityName(String applicationEntityName) {
		this.applicationEntityName = applicationEntityName;
	}
}
