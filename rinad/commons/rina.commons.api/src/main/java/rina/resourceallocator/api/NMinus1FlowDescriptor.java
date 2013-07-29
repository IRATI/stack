package rina.resourceallocator.api;

import rina.ipcservice.api.FlowService;
import rina.ipcservice.api.IPCService;
import rina.protection.api.SDUProtectionModule;

/**
 * Describes an N-1 flow allocated by the Resource Allocator, 
 * including port id, the abstract characteristics(flowService) 
 * and the protection modules to be used for that flow
 * @author eduardgrasa
 *
 */
public class NMinus1FlowDescriptor {
	
	public enum Status {NULL, ALLOCATION_REQUESTED, ALLOCATED, DEALLOCATION_REQUESTED};
	
	/**
	 * The port Id of the N-1 flow
	 */
	private int portId = -1;
	
	/**
	 * The FlowService (contains naming information of the endpoints of the flow 
	 * plus the requested QoS parameters)
	 */
	private FlowService flowService = null;
	
	/**
	 * The SDU protection module to be used to protect the SDUs 
	 * that have to be sent through this N-1 flow
	 */
	private SDUProtectionModule sduProtectionModule = null;
	
	private Status status = Status.NULL;
	
	/**
	 * The IPC Process that is handling our request
	 */
	private IPCService ipcService = null;
	
	/**
	 * True if this N-1 flow is used for layer management, 
	 * false otherwise
	 */
	private boolean management = false;
	
	/**
	 * The name of the N-1 DIF through which the N-1 flow is allocated
	 */
	private String nMinus1DIFName = null;

	public int getPortId() {
		return portId;
	}

	public void setPortId(int portId) {
		this.portId = portId;
	}

	public FlowService getFlowService() {
		return flowService;
	}

	public void setFlowService(FlowService flowService) {
		this.flowService = flowService;
	}

	public SDUProtectionModule getSduProtectionModule() {
		return sduProtectionModule;
	}

	public void setSduProtectionModule(SDUProtectionModule sduProtectionModule) {
		this.sduProtectionModule = sduProtectionModule;
	}
	
	public Status getStatus() {
		return status;
	}

	public void setStatus(Status status) {
		this.status = status;
	}

	public IPCService getIpcService() {
		return ipcService;
	}

	public void setIpcService(IPCService ipcService) {
		this.ipcService = ipcService;
	}

	public boolean isManagement() {
		return this.management;
	}

	public void setManagement(boolean management) {
		this.management = management;
	}

	public String getnMinus1DIFName() {
		return nMinus1DIFName;
	}

	public void setnMinus1DIFName(String nMinus1DIFName) {
		this.nMinus1DIFName = nMinus1DIFName;
	}

	public String toString(){
		String result = "Port Id: "+portId+";  Is Management: " + this.management + "; Status: "+this.status+ "\n";
		result = result + "N-1 DIF name: " + this.nMinus1DIFName + "\n";
		result = result + "Flow service: "+flowService.toString();
		if (this.sduProtectionModule != null){
			result = result + "SDU Protection: "+this.sduProtectionModule.toString();
		}
		
		return result;
	}
	
	@Override
	public boolean equals(Object object){
		if (object == null){
			return false;
		}

		if (!(object instanceof NMinus1FlowDescriptor)){
			return false;
		}

		NMinus1FlowDescriptor candidate = (NMinus1FlowDescriptor) object;

		if (candidate.getFlowService() == null){
			return false;
		}

		return (candidate.getFlowService().getPortId() == this.getFlowService().getPortId());
	}

}
