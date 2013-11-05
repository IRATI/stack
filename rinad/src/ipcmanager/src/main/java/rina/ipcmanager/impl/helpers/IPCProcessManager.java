package rina.ipcmanager.impl.helpers;

import java.util.List;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import rina.ipcmanager.impl.IPCManager;
import rina.ipcmanager.impl.conf.DIFProperties;
import rina.ipcmanager.impl.conf.RINAConfiguration;
import eu.irati.librina.ApplicationProcessNamingInformation;
import eu.irati.librina.AssignToDIFException;
import eu.irati.librina.AssignToDIFResponseEvent;
import eu.irati.librina.CreateIPCProcessException;
import eu.irati.librina.DIFConfiguration;
import eu.irati.librina.DIFInformation;
import eu.irati.librina.IPCException;
import eu.irati.librina.IPCProcess;
import eu.irati.librina.IPCProcessFactorySingleton;
import eu.irati.librina.IPCProcessPointerVector;
import eu.irati.librina.UpdateDIFConfigurationResponseEvent;

public class IPCProcessManager {
	
	private IPCProcessFactorySingleton ipcProcessFactory = null;
	private Map<Long, PendingDIFAssignment> pendingDIFAssignments = null;
	private Map<Long, PendingDIFConfiguration> pendingDIFConfigurations = null;
	private static final Log log = 
			LogFactory.getLog(IPCProcessManager.class);
	
	public IPCProcessManager(IPCProcessFactorySingleton ipcProcessFactory) {
		this.ipcProcessFactory = ipcProcessFactory;
		this.pendingDIFAssignments = 
				new ConcurrentHashMap<Long, PendingDIFAssignment>();
		this.pendingDIFConfigurations = 
				new ConcurrentHashMap<Long, PendingDIFConfiguration>();
	}
	
	public synchronized IPCProcessPointerVector listIPCProcesses(){
		return ipcProcessFactory.listIPCProcesses();
	}
	
	public synchronized IPCProcess selectIPCProcessWithFlow(int portId) throws Exception{
		IPCProcessPointerVector ipcProcesses = listIPCProcesses();
		IPCProcess ipcProcess = null;
		
		for(int i=0; i<ipcProcesses.size(); i++) {
			ipcProcess = ipcProcesses.get(i);
			try{
				ipcProcess.getFlowInformation(portId);
				return ipcProcess;
			}catch(IPCException ex){
				continue;
			}
		}
		
		throw new Exception("Could not find IPC Process wifh a flow with portId "+portId);
	}
	
	/**
	 * 1 Look for a normal IPC Process, otherwise for a shim Ethernet one, otherwise
	 * for a shim dummy one
	 * @return
	 */
	public synchronized IPCProcess selectAnyIPCProcess() throws Exception{
		IPCProcessPointerVector ipcProcesses = ipcProcessFactory.listIPCProcesses();
		if (ipcProcesses.size() == 0){
			throw new Exception("Could not find IPC Process to register at");
		}
		
		IPCProcess ipcProcess = null;
		for(int i=0; i<ipcProcesses.size(); i++){
			ipcProcess = ipcProcesses.get(i);
			if (ipcProcess.getType().equals(IPCManager.NORMAL_IPC_PROCESS_TYPE)){
				return ipcProcess;
			}
		}
		
		for(int i=0; i<ipcProcesses.size(); i++){
			ipcProcess = ipcProcesses.get(i);
			if (ipcProcess.getType().equals(IPCManager.SHIM_ETHERNET_IPC_PROCESS_TYPE)){
				return ipcProcess;
			}
		}
		
		for(int i=0; i<ipcProcesses.size(); i++){
			ipcProcess = ipcProcesses.get(i);
			if (ipcProcess.getType().equals(IPCManager.SHIM_DUMMY_IPC_PROCESS_TYPE)){
				return ipcProcess;
			}
		}
		
		throw new Exception("Could not find any IPC Process");
	}
	
	public synchronized IPCProcess selectIPCProcessOfDIF(String difName) throws Exception{
		IPCProcessPointerVector ipcProcesses = ipcProcessFactory.listIPCProcesses();
		IPCProcess ipcProcess = null;
		
		for(int i=0; i<ipcProcesses.size(); i++){
			ipcProcess = ipcProcesses.get(i);
			DIFInformation difInformation = ipcProcess.getDIFInformation();
			if (difInformation != null && 
					difInformation.getDifName().getProcessName().equals(difName)){
				return ipcProcess;
			}
		}
		
		throw new Exception("Could not find IPC Process belonging to DIF "+difName);
	}
	
	public synchronized IPCProcess getIPCProcessNotInList(List<String> ipcProcessList){
		IPCProcessPointerVector ipcProcesses = ipcProcessFactory.listIPCProcesses();
		for(int i=0; i<ipcProcesses.size(); i++) {
			if (!ipcProcessList.contains(ipcProcesses.get(i).getName().toString())){
				return ipcProcesses.get(i);
			}
		}
		
		return null;
	}
	
	public String getIPCProcessesInformationAsString(){
		IPCProcessPointerVector ipcProcesses = ipcProcessFactory.listIPCProcesses();
		IPCProcess ipcProcess = null;
		DIFInformation difInformation = null;
		String result = "";
		
		for(int i=0; i<ipcProcesses.size(); i++){
			ipcProcess = ipcProcesses.get(i);
			result = result + "Id: "+ ipcProcess.getId() + "\n";
			result = result + "    Type: " + ipcProcess.getType() + "\n";
			result = result + "    Name: " + ipcProcess.getName().toString() + "\n";
			difInformation = ipcProcess.getDIFInformation();
			if (difInformation != null){
				result = result + "    Member of DIF: " + 
						difInformation.getDifName().getProcessName() + "\n"; 
			}
		}
		
		return result;
	}
	
	public synchronized IPCProcess createIPCProcess(
			ApplicationProcessNamingInformation name, 
			String type) throws CreateIPCProcessException {
		return ipcProcessFactory.create(name, type);
	}
	
	public synchronized void destroyIPCProcess(long ipcProcessId)
			throws Exception{
		ipcProcessFactory.destroy(ipcProcessId);
	}
	
	public synchronized IPCProcess getIPCProcess(long ipcProcessId) 
			throws Exception{
		return ipcProcessFactory.getIPCProcess(ipcProcessId);
	}
	
	public synchronized long requestAssignToDIF(
			long ipcProcessID, String difName) throws Exception{
		IPCProcess ipcProcess = getIPCProcess(ipcProcessID);
		return requestAssignToDIF(ipcProcess, difName);
	}

	private long requestAssignToDIF(IPCProcess ipcProcess, String difName) 
			throws AssignToDIFException{
		DIFInformation difInformation = new DIFInformation();
		ApplicationProcessNamingInformation difNamingInfo = 
				new ApplicationProcessNamingInformation();
		difNamingInfo.setProcessName(difName);
		difInformation.setDifName(difNamingInfo);
		difInformation.setDifType(ipcProcess.getType());

		DIFProperties difProperties = 
				RINAConfiguration.getInstance().getDIFConfiguration(difName);
		if (difProperties != null && difProperties.getConfigParameters() != null){
			for(int j=0; j<difProperties.getConfigParameters().size(); j++){
				difInformation.getDifConfiguration().addParameter(
						difProperties.getConfigParameters().get(j));
			}
		}

		long handle = ipcProcess.assignToDIF(difInformation);
		pendingDIFAssignments.put(handle, new PendingDIFAssignment(difInformation, ipcProcess));
		log.debug("Requested the assignment of IPC Process "+ipcProcess.getId() 
				+ " to DIF "+difName);
		
		return handle;
	}
	
	public synchronized void assignToDIFResponse(
			AssignToDIFResponseEvent event) throws Exception{
		IPCProcess ipcProcess = null;
		PendingDIFAssignment pendingDIFAssignment = null;
		boolean success;

		pendingDIFAssignment = pendingDIFAssignments.remove(event.getSequenceNumber());
		if (pendingDIFAssignment == null){
			throw new Exception("Could not find a pending DIF assignment with the handle "
					+event.getSequenceNumber());
		}

		ipcProcess = pendingDIFAssignment.getIpcProcess();
		if (event.getResult() == 0){
			success = true;
		}else{
			success = false;
		}

		try {
			ipcProcess.assignToDIFResult(success);
			if (success){
				log.info("Successfully assigned IPC Process "+ ipcProcess.getId() + 
						" to DIF "+ pendingDIFAssignment.getDIFInformation().getDifName().getProcessName());
			} else {
				log.info("Could not assign IPC Process "+ ipcProcess.getId() + 
						" to DIF "+ pendingDIFAssignment.getDIFInformation().getDifName().getProcessName());
			}
		}catch(Exception ex){
			log.error("Problems processing AssignToDIFResponseEvent. Handle: "+event.getSequenceNumber() + 
					"; DIF name: " + pendingDIFAssignment.getDIFInformation().getDifName().getProcessName());
		}
	}
	
	public synchronized long requestUpdateDIFConfiguration(long ipcProcessID,
			DIFConfiguration difConfiguration) throws Exception{
		IPCProcess ipcProcess = this.ipcProcessFactory.getIPCProcess(ipcProcessID);
		long handle = ipcProcess.updateDIFConfiguration(difConfiguration);
		pendingDIFConfigurations.put(handle, new PendingDIFConfiguration(difConfiguration, ipcProcess));
		log.debug("Requested the configuration of IPC Process "+ipcProcess.getId());
		return handle;
	}

	public synchronized void updateDIFConfigurationResponse(
			UpdateDIFConfigurationResponseEvent event) throws Exception{
		IPCProcess ipcProcess = null;
		PendingDIFConfiguration pendingDIFConfiguration = null;
		boolean success;

		pendingDIFConfiguration = pendingDIFConfigurations.remove(event.getSequenceNumber());
		if (pendingDIFConfiguration == null){
			throw new Exception("Could not find a pending DIF configuration with the handle "
					+event.getSequenceNumber());
		}

		ipcProcess = pendingDIFConfiguration.getIpcProcess();
		if (event.getResult() == 0){
			success = true;
		}else{
			success = false;
		}

		try {
			ipcProcess.updateDIFConfigurationResult(success);
			if (success){
				log.info("Successfully configured IPC Process "+ ipcProcess.getId());
			} else {
				log.info("Could not configure IPC Process "+ ipcProcess.getId());
			}
		}catch(Exception ex){
			log.error("Problems processing UpdateDIFConfigurationResponseEvent. Handle: "+
					event.getSequenceNumber());
		};
	}

}
