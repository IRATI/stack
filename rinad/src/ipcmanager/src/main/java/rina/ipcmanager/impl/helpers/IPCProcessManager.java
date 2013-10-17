package rina.ipcmanager.impl.helpers;

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
import eu.irati.librina.IPCProcess;
import eu.irati.librina.IPCProcessFactorySingleton;
import eu.irati.librina.IPCProcessPointerVector;

public class IPCProcessManager {
	
	private IPCProcessFactorySingleton ipcProcessFactory = null;
	private Map<Long, PendingDIFAssignment> pendingDIFAssignments = null;
	private static final Log log = 
			LogFactory.getLog(IPCProcessManager.class);
	
	public IPCProcessManager(IPCProcessFactorySingleton ipcProcessFactory) {
		this.ipcProcessFactory = ipcProcessFactory;
		this.pendingDIFAssignments = 
				new ConcurrentHashMap<Long, PendingDIFAssignment>();
	}
	
	public synchronized IPCProcessPointerVector listIPCProcesses(){
		return ipcProcessFactory.listIPCProcesses();
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
		
		throw new Exception("Could not find IPC Process to register at");
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
	
	public synchronized void requestAssignToDIF(
			long ipcProcessID, String difName) throws Exception{
		IPCProcess ipcProcess = getIPCProcess(ipcProcessID);
		requestAssignToDIF(ipcProcess, difName);
	}

	private void requestAssignToDIF(IPCProcess ipcProcess, String difName) 
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
	}
	
	public synchronized void assignToDIFResponse(
			AssignToDIFResponseEvent event) throws Exception{
		IPCProcess ipcProcess = null;
		PendingDIFAssignment pendingDIFAssignment = null;
		boolean success;

		pendingDIFAssignment = pendingDIFAssignments.remove(event.getSequenceNumber());
		if (pendingDIFAssignment == null){
			throw new Exception("Could not find a pending DIF assignment to the handle "
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

	public void updateDIFConfiguration(long ipcProcessID,
			DIFConfiguration difConfiguration) throws Exception{
		IPCProcess ipcProcess = this.ipcProcessFactory.getIPCProcess(ipcProcessID);
		ipcProcess.updateDIFConfiguration(difConfiguration);
	}

}
