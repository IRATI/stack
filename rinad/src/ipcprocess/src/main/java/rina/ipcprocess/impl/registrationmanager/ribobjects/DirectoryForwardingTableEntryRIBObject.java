package rina.ipcprocess.impl.registrationmanager.ribobjects;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import eu.irati.librina.ApplicationProcessNamingInformation;

import rina.cdap.api.CDAPSessionDescriptor;
import rina.cdap.api.message.CDAPMessage;
import rina.flowallocator.api.DirectoryForwardingTableEntry;
import rina.ipcprocess.api.IPCProcess;
import rina.registrationmanager.api.RegistrationManager;
import rina.ribdaemon.api.NotificationPolicy;
import rina.ribdaemon.api.RIBDaemonException;
import rina.ribdaemon.api.SimpleSetMemberRIBObject;

public class DirectoryForwardingTableEntryRIBObject extends SimpleSetMemberRIBObject{
	
	private static final Log log = LogFactory.getLog(DirectoryForwardingTableEntryRIBObject.class);
	
	RegistrationManager registrationManager = null;
	ApplicationProcessNamingInformation apNameEntry = null;
	
	public DirectoryForwardingTableEntryRIBObject(IPCProcess ipcProcess, String objectName, 
			DirectoryForwardingTableEntry directoryForwardingTableEntry){
		super(ipcProcess, DirectoryForwardingTableEntrySetRIBObject.DIRECTORY_FORWARDING_TABLE_ENTRY_RIB_OBJECT_CLASS, 
				objectName, directoryForwardingTableEntry);
		setRIBDaemon(ipcProcess.getRIBDaemon());
		registrationManager = ipcProcess.getRegistrationManager();
		registrationManager.addDFTEntry(directoryForwardingTableEntry);
		apNameEntry = directoryForwardingTableEntry.getApNamingInfo();
	}
	
	@Override
	public void create(CDAPMessage cdapMessage, CDAPSessionDescriptor cdapSessionDescriptor) throws RIBDaemonException{
		DirectoryForwardingTableEntry entry = null;
		DirectoryForwardingTableEntry currentEntry = registrationManager.getDFTEntry(apNameEntry);
		
		//Decode the object
		try{
			entry = (DirectoryForwardingTableEntry) this.getEncoder().decode(cdapMessage.getObjValue().getByteval(), 
					DirectoryForwardingTableEntry.class);
		}catch(Exception ex){
			throw new RIBDaemonException(RIBDaemonException.PROBLEMS_DECODING_OBJECT, ex.getMessage());
		}
		
		if (!entry.getKey().equals(apNameEntry.getEncodedString())){
			log.error("This entry cannot be updated by the received value.\n Received entry key: "
					+entry.getKey() + "\n Current entry key: " + apNameEntry.getEncodedString());
			return;
		}
		
		//See if the entry value actually changes, if so call the ribdaemon causing it to notify
		if (!currentEntry.equals(entry)){
			try{
				registrationManager.addDFTEntry(entry);
				NotificationPolicy notificationObject = new NotificationPolicy(new int[]{cdapSessionDescriptor.getPortId()});
				this.getRIBDaemon().create(cdapMessage.getObjClass(), cdapMessage.getObjInst(),
						cdapMessage.getObjName(), entry, notificationObject);
			}catch(RIBDaemonException ex){
				log.error(ex.getMessage());
				ex.printStackTrace();
			}
		}
	}
	
	@Override
	public void delete(CDAPMessage cdapMessage, CDAPSessionDescriptor cdapSessionDescriptor) throws RIBDaemonException{
		try{
			NotificationPolicy notificationObject = new NotificationPolicy(new int[]{cdapSessionDescriptor.getPortId()});
			this.getRIBDaemon().delete(cdapMessage.getObjClass(), cdapMessage.getObjInst(),  
					cdapMessage.getObjName(), null, notificationObject);
		}catch(RIBDaemonException ex){
			log.error(ex.getMessage());
			ex.printStackTrace();
		}
	}
	
	@Override
	public void delete(Object object) throws RIBDaemonException {
		registrationManager.removeDFTEntry(apNameEntry);
		this.getParent().removeChild(this.getObjectName());
		this.getRIBDaemon().removeRIBObject(this);
	}
	
	@Override
	public synchronized Object getObjectValue() {
		return registrationManager.getDFTEntry(apNameEntry);
	}
}
