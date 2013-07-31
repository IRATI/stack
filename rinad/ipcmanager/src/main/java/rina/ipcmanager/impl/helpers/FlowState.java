package rina.ipcmanager.impl.helpers;

import eu.irati.librina.ApplicationProcessNamingInformation;
import eu.irati.librina.FlowRequestEvent;
import eu.irati.librina.FlowSpecification;

/**
 * Captures the state of a flow as required by the IPC Manager
 * @author eduardgrasa
 *
 */
public class FlowState {

	/** The name of the local application involved in the flow */
	private ApplicationProcessNamingInformation localApplication;
	
	/** The name of the remote application involved in the flow */ 
	private ApplicationProcessNamingInformation remoteApplication;
	
	/** The flow characteristics */
	private FlowSpecification flowSpecification;
	
	/** The portId of the flow */
	private int portId;
	
	/** The DIF Name */
	private String difName;
	
	/** The Id of the IPC Process that has allocated the flow */
	private long ipcProcessId;
	
	public FlowState(){
	}
	
	public FlowState(FlowRequestEvent event){
		this.localApplication = event.getLocalApplicationName();
		this.remoteApplication = event.getRemoteApplicationName();
		this.flowSpecification = event.getFlowSpecification();
		this.portId = event.getPortId();
	}

	public ApplicationProcessNamingInformation getLocalApplication() {
		return localApplication;
	}

	public void setLocalApplication(
			ApplicationProcessNamingInformation localApplication) {
		this.localApplication = localApplication;
	}

	public ApplicationProcessNamingInformation getRemoteApplication() {
		return remoteApplication;
	}

	public void setRemoteApplication(
			ApplicationProcessNamingInformation remoteApplication) {
		this.remoteApplication = remoteApplication;
	}

	public FlowSpecification getFlowSpecification() {
		return flowSpecification;
	}

	public void setFlowSpecification(FlowSpecification flowSpecification) {
		this.flowSpecification = flowSpecification;
	}

	public int getPortId() {
		return portId;
	}

	public void setPortId(int portId) {
		this.portId = portId;
	}

	public String getDifName() {
		return difName;
	}

	public void setDifName(String difName) {
		this.difName = difName;
	}

	public long getIpcProcessId() {
		return ipcProcessId;
	}

	public void setIpcProcessId(long ipcProcessId) {
		this.ipcProcessId = ipcProcessId;
	}
}
