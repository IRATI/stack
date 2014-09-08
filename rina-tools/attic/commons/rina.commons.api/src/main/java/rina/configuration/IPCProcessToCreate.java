package rina.configuration;

import java.util.List;
import java.util.Map;

/**
 * Contains enough information to create an IPC Process
 * @author eduardgrasa
 *
 */
public class IPCProcessToCreate {
	
	private String type = null;
	private String applicationProcessName = null;
	private String applicationProcessInstance = null;
	private String difName = null;
	private List<NeighborData> neighbors = null;
	private List<String> difsToRegisterAt = null;
	private String hostname = null;
	private List<SDUProtectionOption> sduProtectionOptions = null;
	private Map<String, String> parameters = null;
	
	public String getType() {
		return type;
	}
	
	public void setType(String type) {
		this.type = type;
	}
	
	public String getHostname() {
		return hostname;
	}
	
	public void setHostname(String hostname) {
		this.hostname = hostname;
	}
	
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
	
	public String getDifName() {
		return difName;
	}
	
	public void setDifName(String difName) {
		this.difName = difName;
	}
	
	public List<NeighborData> getNeighbors() {
		return neighbors;
	}
	
	public void setNeighbors(List<NeighborData> neighbors) {
		this.neighbors = neighbors;
	}
	
	public Map<String, String> getParameters() {
		return parameters;
	}
	
	public void setParameters(Map<String, String> parameters) {
		this.parameters = parameters;
	}
	
	public List<String> getDifsToRegisterAt() {
		return difsToRegisterAt;
	}
	
	public void setDifsToRegisterAt(List<String> difsToRegisterAt) {
		this.difsToRegisterAt = difsToRegisterAt;
	}

	public List<SDUProtectionOption> getSduProtectionOptions() {
		return sduProtectionOptions;
	}

	public void setSduProtectionOptions(
			List<SDUProtectionOption> sduProtectionOptions) {
		this.sduProtectionOptions = sduProtectionOptions;
	}
}
