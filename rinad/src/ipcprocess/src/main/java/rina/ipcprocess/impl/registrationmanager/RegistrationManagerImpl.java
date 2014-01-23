package rina.ipcprocess.impl.registrationmanager;

import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import eu.irati.librina.ApplicationProcessNamingInformation;
import eu.irati.librina.ApplicationRegistrationRequestEvent;
import eu.irati.librina.ApplicationUnregistrationRequestEvent;
import eu.irati.librina.rina;

import rina.flowallocator.api.DirectoryForwardingTableEntry;
import rina.ipcprocess.api.IPCProcess;
import rina.ipcprocess.impl.registrationmanager.ribobjects.DirectoryForwardingTableEntrySetRIBObject;
import rina.registrationmanager.api.RegistrationManager;
import rina.ribdaemon.api.NotificationPolicy;
import rina.ribdaemon.api.RIBDaemon;
import rina.ribdaemon.api.RIBDaemonException;
import rina.ribdaemon.api.RIBObject;
import rina.ribdaemon.api.RIBObjectNames;

public class RegistrationManagerImpl implements RegistrationManager {
	
	private static final Log log = LogFactory.getLog(RegistrationManagerImpl.class);
	
	private Map<String, DirectoryForwardingTableEntry> directoryForwardingTable = null;
	private IPCProcess ipcProcess = null;
	private RIBDaemon ribDaemon = null;
	
	/** The registered applications */
	private Map<String, ApplicationProcessNamingInformation> registeredApplications = null;

	/**
	 * @param args
	 */
	public RegistrationManagerImpl() {
		directoryForwardingTable  = new ConcurrentHashMap<String, DirectoryForwardingTableEntry>();
		registeredApplications = new ConcurrentHashMap<String, ApplicationProcessNamingInformation>();
	}
	
	public void setIPCProcess(IPCProcess ipcProcess){
		this.ipcProcess = ipcProcess;
		ribDaemon = ipcProcess.getRIBDaemon();
		populateRIB(ipcProcess);
	}
	
	private void populateRIB(IPCProcess ipcProcess){
		try{
			RIBObject ribObject = new DirectoryForwardingTableEntrySetRIBObject(ipcProcess);
			ribDaemon.addRIBObject(ribObject);
		}catch(RIBDaemonException ex){
			ex.printStackTrace();
			log.error("Could not subscribe to RIB Daemon:" +ex.getMessage());
		}
	}
	
	public long getAddress(ApplicationProcessNamingInformation apNamingInfo) {
		if (apNamingInfo == null) {
			log.error("Bogus apNamingInfo passed");
			return 0L;
		}
		
		DirectoryForwardingTableEntry entry = directoryForwardingTable.get(
				apNamingInfo.getEncodedString());
        if (entry!= null){
                return entry.getAddress();
        }else{
                return 0L;
        }
	}
	
	public void addDFTEntry(DirectoryForwardingTableEntry entry) {
		if (entry == null || entry.getApNamingInfo() == null) {
			log.error("Tried to put a malformed entry to the directory forwarding table");
			return;
		}
		
		directoryForwardingTable.put(entry.getKey(), entry);
		log.debug("Added entry to the Directory Forwarding Table: \n" 
				+ entry.toString());
	}
	
	public DirectoryForwardingTableEntry getDFTEntry(
			ApplicationProcessNamingInformation apNamingInfo) {
		if (apNamingInfo == null) {
			log.error("Bogus apNamingInfo passed");
		}
		
		return directoryForwardingTable.get(apNamingInfo.getEncodedString());
	}
	
	public void removeDFTEntry(ApplicationProcessNamingInformation apNamingInfo) {
		if (apNamingInfo == null) {
			log.error("Bogus apNamingInfo passed");
		}
		
		DirectoryForwardingTableEntry entry = 
				directoryForwardingTable.remove(apNamingInfo.getEncodedString());
		 if (entry != null) {
			 log.debug("Removed entriy from the Directory Forwarding Table: \n" 
					 + entry.toString());
		 }
	}
	
	public synchronized void processApplicationRegistrationRequestEvent(
			ApplicationRegistrationRequestEvent event) {
		if (event.getApplicationRegistrationInformation() == null || 
				event.getApplicationRegistrationInformation().getApplicationName() == null) {
			log.error("Received Application Registration Request Event with bogus parameters");
			try {
				rina.getExtendedIPCManager().registerApplicationResponse(event, -1);
			} catch (Exception ex) {
				log.error("Problems communicating with the IPC Manager: "+ex.getMessage());
			}
			return;
		}
		
		ApplicationProcessNamingInformation appToRegister = 
				event.getApplicationRegistrationInformation().getApplicationName();
		if (registeredApplications.get(appToRegister.getEncodedString()) != null) {
			log.error("Application " + appToRegister.getEncodedString() 
					+ " is already registered in this IPC Process");
			try {
				rina.getExtendedIPCManager().registerApplicationResponse(event, -1);
			} catch (Exception ex) {
				log.error("Problems communicating with the IPC Manager: "+ex.getMessage());
			}
			return;
		}
		
		registeredApplications.put(appToRegister.getEncodedString(), appToRegister);
		log.info("Successfully registered application "+appToRegister.getEncodedString() 
				+ ". Notifying the IPC Manager and informing neighbours");
		
		try {
			rina.getExtendedIPCManager().registerApplicationResponse(event, 0);
		}catch (Exception ex) {
			log.error("Problems communicating with the IPC Manager: "+ex.getMessage());
			registeredApplications.remove(appToRegister.getEncodedString());
			return;
		}
		
		try{
			DirectoryForwardingTableEntry entry = new DirectoryForwardingTableEntry();
			entry.setAddress(ipcProcess.getAddress().longValue());
			entry.setApNamingInfo(appToRegister);
			entry.setTimestamp(System.nanoTime()/1000L);

			NotificationPolicy notificationPolicy = new NotificationPolicy(new int[0]);
			ribDaemon.create(DirectoryForwardingTableEntrySetRIBObject.DIRECTORY_FORWARDING_TABLE_ENTRY_RIB_OBJECT_CLASS, 
					DirectoryForwardingTableEntrySetRIBObject.DIRECTORY_FORWARDING_ENTRY_SET_RIB_OBJECT_NAME 
					+ RIBObjectNames.SEPARATOR + appToRegister.getEncodedString(), entry, notificationPolicy);
		}catch(RIBDaemonException ex){
			log.error("Error calling RIB Daemon to disseminate application registration: " 
					+ ex.getMessage());
		}
	}
	
	public void processApplicationUnregistrationRequestEvent(
			ApplicationUnregistrationRequestEvent event) {
		if (event.getApplicationName() == null ) {
			log.error("Received Application Unegistration Request Event with bogus parameters");
			try {
				rina.getExtendedIPCManager().unregisterApplicationResponse(event, -1);
			} catch (Exception ex) {
				log.error("Problems communicating with the IPC Manager: "+ex.getMessage());
			}
			return;
		}
		
		ApplicationProcessNamingInformation unregisteredApp = 
				registeredApplications.remove(
						event.getApplicationName().getEncodedString());
		
		if (unregisteredApp == null) {
			log.error("Application " + event.getApplicationName().getEncodedString()
					+ " is not registered in this IPC Process");
			try {
				rina.getExtendedIPCManager().unregisterApplicationResponse(event, -1);
			} catch (Exception ex) {
				log.error("Problems communicating with the IPC Manager: "+ex.getMessage());
			}
			return;
		}
		
		log.info("Successfully unregistered application "+unregisteredApp.getEncodedString() 
				+ ". Notifying the IPC Manager and informing neighbours");
		
		try {
			rina.getExtendedIPCManager().unregisterApplicationResponse(event, 0);
		}catch (Exception ex) {
			log.error("Problems communicating with the IPC Manager: "+ex.getMessage());
			registeredApplications.put(unregisteredApp.getEncodedString(), unregisteredApp);
			return;
		}
		
		try{
			NotificationPolicy notificationPolicy = new NotificationPolicy(new int[0]);
			ribDaemon.delete(DirectoryForwardingTableEntrySetRIBObject.DIRECTORY_FORWARDING_TABLE_ENTRY_RIB_OBJECT_CLASS, 
					DirectoryForwardingTableEntrySetRIBObject.DIRECTORY_FORWARDING_ENTRY_SET_RIB_OBJECT_NAME 
					+ RIBObjectNames.SEPARATOR + unregisteredApp.getEncodedString(), null, notificationPolicy);
		}catch(RIBDaemonException ex){
			log.error("Error calling RIB Daemon to disseminate application unregistration: " 
					+ ex.getMessage());
		}
	}

}
