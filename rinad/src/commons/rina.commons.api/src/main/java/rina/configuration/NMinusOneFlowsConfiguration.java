package rina.configuration;

import java.util.ArrayList;
import java.util.List;

public class NMinusOneFlowsConfiguration {

	/**
	 * The N-1 QoS id required by a management flow
	 */
	private int managementFlowQoSId = 2;
	
	/**
	 * The N-1 QoS id required by each N-1 flow 
	 * that has to be created by the enrollee IPC Process after 
	 * enrollment has been successfully completed
	 */
	private List<Integer> dataFlowsQoSIds = new ArrayList<Integer>();

	public int getManagementFlowQoSId() {
		return managementFlowQoSId;
	}

	public void setManagementFlowQoSId(int managementFlowQoSId) {
		this.managementFlowQoSId = managementFlowQoSId;
	}

	public List<Integer> getDataFlowsQoSIds() {
		return dataFlowsQoSIds;
	}

	public void setDataFlowsQoSIds(List<Integer> dataFlowsQoSIds) {
		this.dataFlowsQoSIds = dataFlowsQoSIds;
	}
}
