package rina.ipcmanager.impl.helpers;

import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import rina.configuration.RINAConfiguration;
import rina.ipcmanager.impl.IPCManager;

import eu.irati.librina.ApplicationManagerSingleton;
import eu.irati.librina.ApplicationProcessNamingInformation;
import eu.irati.librina.ApplicationProcessNamingInformationListIterator;
import eu.irati.librina.ApplicationRegistrationInformation;
import eu.irati.librina.ApplicationRegistrationRequestEvent;
import eu.irati.librina.ApplicationRegistrationType;
import eu.irati.librina.ApplicationUnregistrationRequestEvent;
import eu.irati.librina.IPCProcess;
import eu.irati.librina.IPCProcessPointerVector;
import eu.irati.librina.IpcmRegisterApplicationResponseEvent;
import eu.irati.librina.IpcmUnregisterApplicationResponseEvent;

/**
 * Manages application registrations:
 * 	 a) Validates registration/unregistration requests
 * 	 b) Maintains the state of established registrations
 * 	 c) Keeps track of what DIFs are available to what applications
 * @author eduardgrasa
 *
 */
public class ApplicationRegistrationManager {
	
	private static final Log log = LogFactory.getLog(ApplicationRegistrationManager.class);
	private IPCProcessManager ipcProcessManager = null;
	private ApplicationManagerSingleton applicationManager = null;
	private Map<Long, PendingRegistration> pendingRegistrations = null;
	private Map<Long, PendingUnregistration> pendingUnregistrations = null;

	public ApplicationRegistrationManager(IPCProcessManager ipcProcessManager, 
			ApplicationManagerSingleton applicationManager){
		this.ipcProcessManager = ipcProcessManager;
		this.applicationManager = applicationManager;
		pendingRegistrations = 
				new ConcurrentHashMap<Long, PendingRegistration>();
		pendingUnregistrations = 
				new ConcurrentHashMap<Long, PendingUnregistration>();
	}
	
	/**
	 * Selects an IPC Process where to register the application, invokes the register application 
	 * method of the IPC Process and updates the aplicationRegistration state
	 * @param event
	 * @param ipcProcessId if the application to be registered is an IPC Process, this is the 
	 * identifier of the IPC process. Otherwise it has the value of NO_IPC_PROCESS_ID
	 * @returns 0 if the registered application is not an IPC Process, the operation handle 
	 * otherwise
	 * @throws Exception
	 */
	public synchronized long requestApplicationRegistration(
			ApplicationRegistrationRequestEvent event, long ipcProcessId) throws Exception {
		IPCProcess ipcProcess = null;
		long handle = -1;
		ApplicationProcessNamingInformation applicationName = 
				event.getApplicationRegistrationInformation().getApplicationName();
		
		try{
			ipcProcess = getIPCProcessToRegisterAt(event);
			handle = ipcProcess.registerApplication(event.
					getApplicationRegistrationInformation().getApplicationName(), 
					event.getApplicationRegistrationInformation().getIpcProcessId());
			pendingRegistrations.put(handle, 
					new PendingRegistration(event, ipcProcess, ipcProcessId));
			log.debug("Requested registration of application "+applicationName.toString() 
					+" to DIF "+ipcProcess.getDIFInformation().getDifName().getProcessName() + 
					". Got handle "+handle);
			return handle;
		}catch(Exception ex){
			log.error("Error requesting application registration "+ex.getMessage());
			
			if (ipcProcessId == IPCManager.NO_IPC_PROCESS_ID) {
				applicationManager.applicationRegistered(event, 
						new ApplicationProcessNamingInformation(), -1);
			} else {
				throw ex;
			}
				
			return 0;
		}
	}
	
	/**
	 * Looks for the pending registration with the event sequence number, and updates the 
	 * registration state based on the result
	 * @throws Exception
	 */
	public synchronized void registerApplicationResponse(
			IpcmRegisterApplicationResponseEvent event) throws Exception {
		PendingRegistration pendingRegistration = null;
		IPCProcess ipcProcess = null;
		ApplicationRegistrationRequestEvent appReqEvent = null;
		boolean success;
		
		pendingRegistration = pendingRegistrations.remove(event.getSequenceNumber());
		if (pendingRegistration == null){
			throw new Exception("Could not find a pending registration associated to the handle "
					+event.getSequenceNumber());
		}
		
		ipcProcess = pendingRegistration.getIpcProcess();
		appReqEvent = pendingRegistration.getEvent();
		if (event.getResult() == 0){
			success = true;
		}else{
			success = false;
		}
		
		try {
			ipcProcess.registerApplicationResult(event.getSequenceNumber(), success);
			if (success){
				log.info("Successfully registered application "+ 
						appReqEvent.getApplicationRegistrationInformation().getApplicationName().toString() + 
						" to DIF "+ipcProcess.getDIFInformation().getDifName().toString());
			} else {
				log.info("Could not register application "+ 
						appReqEvent.getApplicationRegistrationInformation().getApplicationName().toString() + 
						" to DIF "+ipcProcess.getDIFInformation().getDifName().toString());
			}

			if (pendingRegistration.isApplicationRegisteredIPCProcess()) {
				ipcProcessManager.registrationToNMinusOneDIFResponse(event);
			} else {
				applicationManager.applicationRegistered(appReqEvent, 
						ipcProcess.getDIFInformation().getDifName(), event.getResult());
			}
		}catch(Exception ex){
			log.error("Problems processing IPCMRegisterApplicationResponseEvent. Handle: "+event.getSequenceNumber() + 
					  "; Application name: "+ appReqEvent.getApplicationRegistrationInformation().getApplicationName().toString() + 
					  "; DIF name: " + ipcProcess.getDIFInformation().getDifName().toString());
			
			if (pendingRegistration.isApplicationRegisteredIPCProcess()) {
				ipcProcessManager.registrationToNMinusOneDIFResponse(event);
			} else {
				applicationManager.applicationRegistered(appReqEvent, 
						ipcProcess.getDIFInformation().getDifName(), -1);
			}
		}
	}
	
	/**
	 * Called when the IPC Manager is informed that the application identified by apName (process + instance) 
	 * has terminated. We have to look for potential registrations of the application and cancel them
	 * @param apName
	 */
	public synchronized void cleanApplicationRegistrations(ApplicationProcessNamingInformation appName){
		IPCProcessPointerVector ipcProcesses = ipcProcessManager.listIPCProcesses();
		IPCProcess ipcProcess = null;
		long handle = 0;
		ApplicationUnregistrationRequestEvent event = null;
		
		for(int i=0; i<ipcProcesses.size(); i++){
			ipcProcess = ipcProcesses.get(i);
			if (isRegisteredAt(ipcProcess, appName)){
				try{
					handle = ipcProcess.unregisterApplication(appName);
					log.debug("Requested unregitration of application "+appName.toString() 
							+" from DIF "+ipcProcess.getDIFInformation().getDifName().toString() + 
							". Got handle "+handle);
					event = new ApplicationUnregistrationRequestEvent(appName, 
							ipcProcess.getDIFInformation().getDifName(), 
							0);
					pendingUnregistrations.put(handle, new PendingUnregistration(event, ipcProcess));
				}catch(Exception ex){
					log.warn("Error requesting application unregistration "+ex.getMessage());
				}
			}
		}
	}
	
	/**
	 * Get application registration (if existent) and cancel it.
	 * @param event
	 * @param ipcProcessId if the application to be registered is an IPC Process, this is the 
	 * identifier of the IPC process. Otherwise it has the value of NO_IPC_PROCESS_ID
	 * @throws Exception
	 */
	public synchronized long requestApplicationUnregistration(
			ApplicationUnregistrationRequestEvent event, long ipcProcessId) throws Exception{
		IPCProcess ipcProcess = null;
		long handle = -1;
		
		try{
			String difName = event.getDIFName().getProcessName();
			ipcProcess = ipcProcessManager.selectIPCProcessOfDIF(difName);
			handle = ipcProcess.unregisterApplication(event.getApplicationName());
			pendingUnregistrations.put(handle, 
					new PendingUnregistration(event, ipcProcess, ipcProcessId));
			log.debug("Requested unregitration of application "+event.getApplicationName().toString() 
					+" from DIF "+ipcProcess.getDIFInformation().getDifName().toString() + 
					". Got handle "+handle);
			return handle;
		}catch(Exception ex){
			log.error("Error unregistering application. "+ex.getMessage());
			
			if (event.getSequenceNumber() > 0) {
				applicationManager.applicationUnregistered(event, -1);
			}
			
			throw ex;
		}
	}
	
	/**
	 * Looks for the pending unregistration with the event sequence number, and updates the 
	 * registration state based on the result
	 * @throws Exception
	 */
	public synchronized void unregisterApplicationResponse(
			IpcmUnregisterApplicationResponseEvent event) throws Exception {
		PendingUnregistration pendingUnregistration = null;
		IPCProcess ipcProcess = null;
		ApplicationUnregistrationRequestEvent appReqEvent = null;
		boolean success;
		
		pendingUnregistration = pendingUnregistrations.remove(event.getSequenceNumber());
		if (pendingUnregistration == null){
			throw new Exception("Could not find a pending unregistration associated to the handle "
					+event.getSequenceNumber());
		}
		
		ipcProcess = pendingUnregistration.getIpcProcess();
		appReqEvent = pendingUnregistration.getEvent();
		if (event.getResult() == 0){
			success = true;
		}else{
			success = false;
		}
		
		try {
			ipcProcess.unregisterApplicationResult(event.getSequenceNumber(), success);
			if (success){
				log.info("Successfully unregistered application "+ 
						appReqEvent.getApplicationName().toString() + 
						" from DIF "+ipcProcess.getDIFInformation().getDifName().toString());
			} else {
				log.info("Could not unregister application "+ 
						appReqEvent.getApplicationName().toString() + 
						" from DIF "+ipcProcess.getDIFInformation().getDifName().toString());
			}
			
			if (pendingUnregistration.isApplicationRegisteredIPCProcess()) {
				ipcProcessManager.unregistrationFromNMinusOneDIFResponse(event);
			} else if (appReqEvent.getSequenceNumber() > 0) {
				applicationManager.applicationUnregistered(appReqEvent, event.getResult());
			}
		}catch(Exception ex){
			log.error("Problems processing IPCMUnregisterApplicationResponseEvent. Handle: "+event.getSequenceNumber() + 
					  "; Application name: "+ appReqEvent.getApplicationName().toString() + 
					  "; DIF name: " + ipcProcess.getDIFInformation().getDifName().toString());
			
			if (pendingUnregistration.isApplicationRegisteredIPCProcess()) {
				ipcProcessManager.unregistrationFromNMinusOneDIFResponse(event);
			} else if (appReqEvent.getSequenceNumber() > 0) {
				applicationManager.applicationUnregistered(appReqEvent, -1);
			}
		}
	}
	
	private IPCProcess getIPCProcessToRegisterAt(ApplicationRegistrationRequestEvent event) 
			throws Exception{
		ApplicationRegistrationInformation info = event.getApplicationRegistrationInformation();
		
		if (info.getRegistrationType() == ApplicationRegistrationType.APPLICATION_REGISTRATION_ANY_DIF){
			String difName = RINAConfiguration.getInstance().getDIFNameAssociatedToApName(
					info.getApplicationName());
			if (difName == null) {
				return ipcProcessManager.selectAnyIPCProcess();
			} else {
				return getIPCProcessOfDIF(difName, info.getApplicationName());
			}
		}
		
		if (info.getRegistrationType() == ApplicationRegistrationType.APPLICATION_REGISTRATION_SINGLE_DIF){
			return getIPCProcessOfDIF(info.getDIFName().getProcessName(), info.getApplicationName());
		}
		
		throw new Exception("Unsupported registration type: "+info.getRegistrationType());
	}
	
	private IPCProcess getIPCProcessOfDIF(String difName, ApplicationProcessNamingInformation apName) 
			throws Exception{
		IPCProcess ipcProcess = ipcProcessManager.selectIPCProcessOfDIF(difName);
		if (isRegisteredAt(ipcProcess, apName)) {
			throw new Exception("Application already registered in DIF "+difName);
		}
		return ipcProcess;
	}
	
	private boolean isRegisteredAt(
			IPCProcess ipcProcess, 
			ApplicationProcessNamingInformation appName) {
		ApplicationProcessNamingInformationListIterator iterator = 
				ipcProcess.getRegisteredApplications().iterator();
		
		ApplicationProcessNamingInformation next = null;
		String encodedAppName = appName.getEncodedString();
		while (iterator.hasNext()){
			next = iterator.next();
			if (next.getEncodedString().equals(encodedAppName)){
				return true;
			}
		}
		
		return false;
	}

}
