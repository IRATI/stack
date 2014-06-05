package rina.ipcmanager.impl.helpers;

import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import rina.configuration.DIFProperties;
import rina.configuration.KnownIPCProcessAddress;
import rina.configuration.RINAConfiguration;
import rina.ipcmanager.impl.IPCManager;
import rina.ipcmanager.impl.console.IPCManagerConsole;
import eu.irati.librina.ApplicationProcessNamingInformation;
import eu.irati.librina.ApplicationRegistrationInformation;
import eu.irati.librina.ApplicationRegistrationRequestEvent;
import eu.irati.librina.ApplicationRegistrationType;
import eu.irati.librina.ApplicationUnregistrationRequestEvent;
import eu.irati.librina.AssignToDIFException;
import eu.irati.librina.AssignToDIFResponseEvent;
import eu.irati.librina.CreateIPCProcessException;
import eu.irati.librina.DIFConfiguration;
import eu.irati.librina.DIFInformation;
import eu.irati.librina.DataTransferConstants;
import eu.irati.librina.EnrollToDIFResponseEvent;
import eu.irati.librina.IPCException;
import eu.irati.librina.IPCProcess;
import eu.irati.librina.IPCProcessFactorySingleton;
import eu.irati.librina.IPCProcessPointerVector;
import eu.irati.librina.IpcmRegisterApplicationResponseEvent;
import eu.irati.librina.IpcmUnregisterApplicationResponseEvent;
import eu.irati.librina.Neighbor;
import eu.irati.librina.NeighborsModifiedNotificationEvent;
import eu.irati.librina.PDUFTableGeneratorConfiguration;
import eu.irati.librina.Parameter;
import eu.irati.librina.QoSCube;
import eu.irati.librina.UpdateDIFConfigurationResponseEvent;

public class IPCProcessManager {
	
	private IPCProcessFactorySingleton ipcProcessFactory = null;
	private Map<Long, PendingDIFAssignment> pendingDIFAssignments = null;
	private Map<Long, PendingDIFConfiguration> pendingDIFConfigurations = null;
	private Map<Long, PendingIPCProcessRegistration> pendingIPCProcessRegistrations = null;
	private Map<Long, PendingEnrollmentOperation> pendingEnrollmentOperations = null;
	private static final Log log = 
			LogFactory.getLog(IPCProcessManager.class);
	private ApplicationRegistrationManager applicationRegistrationManager = null;
	private IPCManagerConsole console = null;
	
	public IPCProcessManager(IPCProcessFactorySingleton ipcProcessFactory, 
			IPCManagerConsole console) {
		this.ipcProcessFactory = ipcProcessFactory;
		this.console = console;
		this.pendingDIFAssignments = 
				new ConcurrentHashMap<Long, PendingDIFAssignment>();
		this.pendingDIFConfigurations = 
				new ConcurrentHashMap<Long, PendingDIFConfiguration>();
		this.pendingIPCProcessRegistrations = 
				new ConcurrentHashMap<Long, PendingIPCProcessRegistration>();
		this.pendingEnrollmentOperations = 
				new ConcurrentHashMap<Long, PendingEnrollmentOperation>();
	}
	
	public void setApplicationRegistrationManager(ApplicationRegistrationManager 
			applicationRegistrationManager) {
		this.applicationRegistrationManager = applicationRegistrationManager;
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
		
		//TODO, remove this. Dirty hack to be able to test flow allocation using a 
		//single system
		for (int i=0; i<ipcProcesses.size(); i++) {
			ipcProcess = ipcProcesses.get(i);
			if (ipcProcess.getName().getProcessName().equals(difName)) {
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
			
			Iterator<ApplicationProcessNamingInformation> iterator = 
					ipcProcess.getSupportingDIFs().iterator();
			if (iterator.hasNext()) {
				result = result + "    Supporting DIFs: " + "\n"; 
			}
			while (iterator.hasNext()){
				result = result + "        " + iterator.next().getProcessName() + "\n";
			}
			
			difInformation = ipcProcess.getDIFInformation();
			if (difInformation != null && difInformation.getDifName() != null && 
					difInformation.getDifName().getProcessName() != null && 
					!difInformation.getDifName().getProcessName().equals("")){
				result = result + "    Member of DIF: " + 
						difInformation.getDifName().getProcessName() + "\n"; 
				if (difInformation.getDifConfiguration() != null) {
					DIFConfiguration difConfiguration = difInformation.getDifConfiguration();
					
					if (!ipcProcess.getType().equals(IPCManager.NORMAL_IPC_PROCESS_TYPE)) {
						if (difConfiguration.getParameters() != null && 
								difConfiguration.getParameters().size() > 0){
							Iterator<Parameter> paramIterator = difConfiguration.getParameters().iterator();
							result = result + "        Parameters: \n";
							Parameter next = null;
							while (paramIterator.hasNext()){
								next = paramIterator.next();
								result = result + "            Name: " + next.getName() 
										+ "; Value: " + next.getValue() + "\n";
							}
						}
						
						result = result + "\n";
						continue;
					}
					
					if (difConfiguration.getAddress() > 0) {
						result = result + "        Address: " + difConfiguration.getAddress() + "\n";
					}
					
					result = result + "            EFCP Configuration: \n";
					
					if (difConfiguration.getEfcpConfiguration().getDataTransferConstants() != null && 
							difConfiguration.getEfcpConfiguration().getDataTransferConstants().isInitialized()) {
						DataTransferConstants dtc = difConfiguration.getEfcpConfiguration().getDataTransferConstants();
						result = result + "            Data Transfer Constants: \n";
						result = result + "                Address length: " + dtc.getAddressLength() + "\n";
						result = result + "                CEP-id length: " + dtc.getCepIdLength() + "\n";
						result = result + "                Length length: " + dtc.getLengthLength() + "\n";
						result = result + "                QoS-id length: " + dtc.getQosIdLenght() + "\n";
						result = result + "                Sequence number length: " + dtc.getSequenceNumberLength()+ "\n";
						result = result + "                Max PDU lifetime: " + dtc.getMaxPduLifetime()+ "\n";
						result = result + "                Max PDU size: " + dtc.getMaxPduSize() + "\n";
					}
					
					Iterator<QoSCube> qosCubeIterator = difConfiguration.getEfcpConfiguration().getQosCubes().iterator();
					if (qosCubeIterator.hasNext()) {
						result = result + "            QoS Cubes: \n";
					}
					QoSCube qosCube = null;
					while (qosCubeIterator.hasNext()){
						qosCube = qosCubeIterator.next();
						result = result + "                Name: " + qosCube.getName() + "\n";
						result = result + "                Id: " + qosCube.getId() + "\n";
						result = result + "                Ordered delivery: " + qosCube.isOrderedDelivery() + "\n";
						result = result + "                Partial delivery: " + qosCube.isOrderedDelivery() + "\n";
						result = result + "                EFCP policies: " + qosCube.getEfcpPolicies();
						result = result + "            --------------------: \n";
					}
					
					if (difConfiguration.getEfcpConfiguration().getDataTransferConstants() != null && 
							difConfiguration.getPduFTableGeneratorConfiguration() != null) 
					{
						result = result + "        PDUFTableGeneratofConfiguration: \n";
						PDUFTableGeneratorConfiguration pduftTableGeneratorConfiguration = difConfiguration.getPduFTableGeneratorConfiguration();
						result = result + "            PDUFTG policy: " + pduftTableGeneratorConfiguration.getPduFtGeneratorPolicy().getName()+"/" 
									+ pduftTableGeneratorConfiguration.getPduFtGeneratorPolicy().getVersion() + "\n";
						if (pduftTableGeneratorConfiguration.getLinkStateRoutingConfiguration() != null) {
							result = result + "            objectMaximumAge: " + 
									pduftTableGeneratorConfiguration.getLinkStateRoutingConfiguration().getObjectMaximumAge() + "\n";
							result = result + "            waitUntilReadCDAP: " + 
									pduftTableGeneratorConfiguration.getLinkStateRoutingConfiguration().getWaitUntilReadCDAP() + "\n";
							result = result + "            waitUntilError: " + 
									pduftTableGeneratorConfiguration.getLinkStateRoutingConfiguration().getWaitUntilError() + "\n";
							result = result + "            waitUntilPDUFTComputation: " + 
									pduftTableGeneratorConfiguration.getLinkStateRoutingConfiguration().getWaitUntilPDUFTComputation() + "\n";
							result = result + "            waitUntilFSODBPropagation: " + 
									pduftTableGeneratorConfiguration.getLinkStateRoutingConfiguration().getWaitUntilFSODBPropagation() + "\n";
							result = result + "            waitUntilAgeIncrement: " +
									pduftTableGeneratorConfiguration.getLinkStateRoutingConfiguration().getWaitUntilAgeIncrement() + "\n";
						}
					}
					
					Iterator<Neighbor> neighborIterator = ipcProcess.getNeighbors().iterator();
					if (neighborIterator.hasNext()) {
						result = result + "        Neighbors: " + "\n";
					}
					while (neighborIterator.hasNext()) {
						result = result + "            " + 
								neighborIterator.next().getName().getProcessNamePlusInstance() + "\n";
					}
				}
			}
			
			result = result + "\n";
		}
		
		return result;
	}
	
	public synchronized IPCProcess createIPCProcess(
			ApplicationProcessNamingInformation name, 
			String type) throws CreateIPCProcessException {
		IPCProcess ipcProcess = ipcProcessFactory.create(name, type);
		if (!type.equals(IPCManager.NORMAL_IPC_PROCESS_TYPE)) {
			ipcProcess.setInitialized();
		}
		
		return ipcProcess;
	}
	
	public synchronized void destroyIPCProcess(int ipcProcessId)
			throws Exception{
		ipcProcessFactory.destroy(ipcProcessId);
	}
	
	public synchronized IPCProcess getIPCProcess(int ipcProcessId) 
			throws Exception{
		return ipcProcessFactory.getIPCProcess(ipcProcessId);
	}
	
	public synchronized long requestAssignToDIF(
			int ipcProcessID, String difName) throws Exception{
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
		if (difProperties == null) {
			throw new AssignToDIFException("Could not find description of DIF " + difName);
		}
		
		if (difProperties.getConfigParameters() != null){
			for(int i=0; i<difProperties.getConfigParameters().size(); i++){
				difInformation.getDifConfiguration().addParameter(
						difProperties.getConfigParameters().get(i));
			}
		}
		
		if (ipcProcess.getType().equals(IPCManager.NORMAL_IPC_PROCESS_TYPE)) {
			KnownIPCProcessAddress address = RINAConfiguration.getInstance().getIPCProcessAddress(
					difName, ipcProcess.getName().getProcessName(), ipcProcess.getName().getProcessInstance());
			if (address == null) {
				throw new AssignToDIFException("Could not assign an address in DIF " + difName + 
						" to IPC Process " + ipcProcess.getName().toString());
			}
			difInformation.getDifConfiguration().setAddress(address.getAddress());
			
			if (difProperties.getDataTransferConstants() != null) {
				difInformation.getDifConfiguration().getEfcpConfiguration().setDataTransferConstants(
						difProperties.getDataTransferConstants());
			}
			
			if (difProperties.getQosCubes() != null) {
				for(int i=0; i<difProperties.getQosCubes().size(); i++) {
					difInformation.getDifConfiguration().getEfcpConfiguration().addQoSCube(
							difProperties.getQosCubes().get(i));
				}
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
	
	public synchronized long requestUpdateDIFConfiguration(int ipcProcessID,
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
	
	public synchronized long requestRegistrationToNMinusOneDIF(int ipcProcessId, String difName) 
			throws Exception{
		IPCProcess ipcProcess = getIPCProcess(ipcProcessId);
		IPCProcess nMinusOneIpcProcess = selectIPCProcessOfDIF(difName);
		ApplicationProcessNamingInformation difNamingInfo = 
				new ApplicationProcessNamingInformation();
		difNamingInfo.setProcessName(difName);
	
		ApplicationRegistrationInformation arInfo = new ApplicationRegistrationInformation(
				ApplicationRegistrationType.APPLICATION_REGISTRATION_SINGLE_DIF);
		arInfo.setApplicationName(ipcProcess.getName());
		arInfo.setIpcProcessId(ipcProcessId);
		arInfo.setDIFName(difNamingInfo);
		ApplicationRegistrationRequestEvent event = 
				new ApplicationRegistrationRequestEvent(arInfo, 0);
		long handle = 
				applicationRegistrationManager.requestApplicationRegistration(event, ipcProcessId);

		pendingIPCProcessRegistrations.put(
				handle, new PendingIPCProcessRegistration(difName, ipcProcess, nMinusOneIpcProcess));
		log.debug("Requested the registration of IPC Process "+ipcProcess.getId() 
				+ " to N-1 DIF "+difName);
		
		return handle;
	}
	
	public synchronized void registrationToNMinusOneDIFResponse(IpcmRegisterApplicationResponseEvent event) {
		PendingIPCProcessRegistration pendingRegistration = 
				pendingIPCProcessRegistrations.remove(event.getSequenceNumber());
		if (pendingRegistration == null) {
			log.error("Could not find pending IPC Process registration associated to handle " + 
					   event.getSequenceNumber());
			return;
		}
		
		if (event.getResult() == 0) {
			try {
				pendingRegistration.getIpcProcess().notifyRegistrationToSupportingDIF(
						pendingRegistration.getnMinusOneIPCProcess().getName(), 
						pendingRegistration.getnMinusOneIPCProcess().getDIFInformation().getDifName());
			} catch(Exception ex) {
				log.error("Problems notifying IPC Process about successfull registration to DIF" 
						+ pendingRegistration.getDifName() + ". "+ex.getMessage());
			}
		} else {
			log.error("Could not register IPC Process "+ pendingRegistration.getIpcProcess().getId() 
					+ " to DIF " + pendingRegistration.getDifName()+ ". Error code: "+event.getResult());
		}
		
		console.responseArrived(event);
	}
	
	public synchronized long requestUnregistrationFromNMinusOneDIF(int ipcProcessId, String difName) 
			throws Exception{
		IPCProcess ipcProcess = getIPCProcess(ipcProcessId);
		IPCProcess nMinusOneIpcProcess = selectIPCProcessOfDIF(difName);
		ApplicationProcessNamingInformation difNamingInfo = 
				new ApplicationProcessNamingInformation();
		difNamingInfo.setProcessName(difName);
	
		ApplicationUnregistrationRequestEvent event = 
				new ApplicationUnregistrationRequestEvent(
						ipcProcess.getName(), difNamingInfo, 0);
		long handle = 
				applicationRegistrationManager.requestApplicationUnregistration(event, ipcProcessId);

		pendingIPCProcessRegistrations.put(
				handle, new PendingIPCProcessRegistration(difName, ipcProcess, nMinusOneIpcProcess));
		log.debug("Requested the unregistration of IPC Process "+ipcProcess.getId() 
				+ " from N-1 DIF "+difName);
		
		return handle;
	}
	
	public synchronized void unregistrationFromNMinusOneDIFResponse(IpcmUnregisterApplicationResponseEvent event) {
		PendingIPCProcessRegistration pendingRegistration = 
				pendingIPCProcessRegistrations.remove(event.getSequenceNumber());
		if (pendingRegistration == null) {
			log.error("Could not find pending IPC Process registration associated to handle " + 
					   event.getSequenceNumber());
			return;
		}
		
		if (event.getResult() == 0) {
			try {
				pendingRegistration.getIpcProcess().notifyUnregistrationFromSupportingDIF(
						pendingRegistration.getnMinusOneIPCProcess().getName(), 
						pendingRegistration.getnMinusOneIPCProcess().getDIFInformation().getDifName());
			} catch(Exception ex) {
				log.error("Problems notifying IPC Process about successful unregistration from DIF" 
						+ pendingRegistration.getDifName() + ". "+ex.getMessage());
			}
		} else {
			log.error("Could not unregister IPC Process "+ pendingRegistration.getIpcProcess().getId() 
					+ " from DIF " + pendingRegistration.getDifName()+ ". Error code: "+event.getResult());
		}
		
		console.responseArrived(event);
	}
	
	public synchronized void setInitialized(int ipcProcessId) throws Exception {
		IPCProcess ipcProcess = this.getIPCProcess(ipcProcessId);
		ipcProcess.setInitialized();
	}
	
	public synchronized long requestEnrollmentToDIF(int ipcProcessId, String difName, 
			String supportingDifName, String neighApName, String neighApInstance) throws Exception {
		IPCProcess ipcProcess = getIPCProcess(ipcProcessId);
		ApplicationProcessNamingInformation difNamingInfo = 
				new ApplicationProcessNamingInformation();
		difNamingInfo.setProcessName(difName);
		ApplicationProcessNamingInformation supportingDifNamingInfo = 
				new ApplicationProcessNamingInformation();
		supportingDifNamingInfo.setProcessName(supportingDifName);
		ApplicationProcessNamingInformation neighborNamingInfo = 
				new ApplicationProcessNamingInformation();
		neighborNamingInfo.setProcessName(neighApName);
		neighborNamingInfo.setProcessInstance(neighApInstance);
		
		long handle = ipcProcess.enroll(difNamingInfo, supportingDifNamingInfo, neighborNamingInfo);
		pendingEnrollmentOperations.put(
				handle, new PendingEnrollmentOperation(ipcProcess, difNamingInfo, 
						supportingDifNamingInfo, neighborNamingInfo));
		log.debug("Requested the enrollment of IPC Process "+ipcProcess.getId() 
				+ " to DIF "+difName + " through N-1 DIF "+ supportingDifName 
				+ ". The point of contact (neighbor) is "+neighborNamingInfo.toString());
		
		return handle;
	}

	public synchronized void enrollToDIFResponse(EnrollToDIFResponseEvent event) {
		PendingEnrollmentOperation operation = 
				pendingEnrollmentOperations.get(event.getSequenceNumber());
		if (operation == null) {
			log.error("Could not find pending IPC Process enroll to DIF operation associated to handle " + 
					   event.getSequenceNumber());
			return;
		}
		
		if (event.getResult() == 0) {
			operation.getIpcProcess().addNeighbors(event.getNeighbors());
			operation.getIpcProcess().setDIFInformation(event.getDIFInformation());
			log.debug("Enrollment operation associated to handle "+event.getSequenceNumber() 
					+" completed successfully.");
		} else {
			log.error("Enrollment operation associated to handle " + event.getSequenceNumber() 
					+ " failed. Error code: "+event.getResult());
		}
		
		console.responseArrived(event);
	}
	
	public synchronized void neighborsModifiedEvent(NeighborsModifiedNotificationEvent event) 
			throws Exception{
		IPCProcess ipcProcess = getIPCProcess(event.getIpcProcessId());
		if (event.getNeighbors() == null) {
			log.warn("Received a neighbors modified event with an empty neighbors list");
			return;
		}
		
		Iterator<Neighbor> iterator = event.getNeighbors().iterator();
		String neighborsString = "";
		while(iterator.hasNext()) {
			neighborsString = neighborsString + iterator.next().getName().getProcessNamePlusInstance() + "\n";
		}
		
		if (event.isAdded()) {
			ipcProcess.addNeighbors(event.getNeighbors());
			log.info("IPC process "+event.getIpcProcessId()+" has the following new neighbors:\n" 
					+ neighborsString);
		} else {
			ipcProcess.removeNeighbors(event.getNeighbors());
			log.info("IPC process "+event.getIpcProcessId()+" has lost the following neighbors:\n" 
					+ neighborsString);
		}
	}
	
	public synchronized long requestQueryRIB(int ipcProcessId) throws Exception {
		IPCProcess ipcProcess = getIPCProcess(ipcProcessId);
		return ipcProcess.queryRIB("", "", 0, 0, "");
	}
}
