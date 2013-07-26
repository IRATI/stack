package rina.ipcprocess.api;

import java.util.List;
import java.util.Map;

import rina.applicationprocess.api.WhatevercastName;
import rina.efcp.api.DataTransferConstants;
import rina.enrollment.api.Neighbor;
import rina.flowallocator.api.Flow;
import rina.flowallocator.api.QoSCube;
import rina.ipcmanager.api.IPCManager;
import rina.applicationprocess.api.ApplicationProcessNamingInfo;

/**
 * Represents an IPC Process. Holds together the different components of the IPC 
 * process
 * @author eduardgrasa
 *
 */
public interface IPCProcess{
	
	/**
	 * The operational status of the IPC Process
	 */
	public enum OperationalStatus {STARTED, STOPPED};
	
	/**
	 * The type of IPC Process
	 * @author eduardgrasa
	 */
	public enum IPCProcessType {FAKE, NORMAL, SHIM_IP};
	
	/**
	 * Return the type of the IPC Process
	 * @return
	 */
	public IPCProcessType getType();
	
	/* IPC Process Component management */
	public Map<String, IPCProcessComponent> getIPCProcessComponents();
	
	public IPCProcessComponent getIPCProcessComponent(String componentName);
	
	public void addIPCProcessComponent(IPCProcessComponent ipcProcessComponent);
	
	public IPCProcessComponent removeIPCProcessComponent(String componentName);
	
	public void setIPCProcessCompnents(Map<String, IPCProcessComponent> ipcProcessComponents);
	
	/* IPC Manager */
	/**
	 * Set the IPCManager of this system
	 * @param ipcManager
	 */
	public void setIPCManager(IPCManager ipcManager);
	
	/**
	 * Get the IPCManager of this system
	 * @return
	 */
	public IPCManager getIPCManager();
	
	/**
	 * Lifecicle event, invoked to tell the IPC process it is about to be destroyed.
	 * The IPC Process implementation must do any necessary cleanup inside this 
	 * operation.
	 */
	public void destroy();
	
	/**
	 * Will call the execute operation of the IPCManager in order to execute a runnable.
	 * Classes implementing IPCProcess should not create its own thread pool, but use 
	 * the one managed by the IPCManager instead.
	 * @param runnable
	 */
	public void execute(Runnable runnable);
	
	/* Convenience methods to get information from the RIB */
	public ApplicationProcessNamingInfo getApplicationProcessNamingInfo();
	public String getApplicationProcessName();
	public String getApplicationProcessInstance();
	public List<WhatevercastName> getWhatevercastNames();
	public String getDIFName();
	public List<Neighbor> getNeighbors();
	public Long getAddress();
	public OperationalStatus getOperationalStatus();
	public List<QoSCube> getQoSCubes();
	public List<Flow> getAllocatedFlows();
	public DataTransferConstants getDataTransferConstants();
}
