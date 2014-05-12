package rina.ipcprocess.impl.registrationmanager.ribobjects;

import java.util.ArrayList;
import java.util.List;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import eu.irati.librina.Neighbor;

import rina.cdap.api.CDAPSessionDescriptor;
import rina.cdap.api.message.CDAPMessage;
import rina.events.api.Event;
import rina.events.api.EventListener;
import rina.flowallocator.api.DirectoryForwardingTableEntry;
import rina.ipcprocess.api.IPCProcess;
import rina.ipcprocess.impl.events.ConnectivityToNeighborLostEvent;
import rina.ribdaemon.api.BaseRIBObject;
import rina.ribdaemon.api.NotificationPolicy;
import rina.ribdaemon.api.ObjectInstanceGenerator;
import rina.ribdaemon.api.RIBDaemonException;
import rina.ribdaemon.api.RIBObject;
import rina.ribdaemon.api.RIBObjectNames;

public class DirectoryForwardingTableEntrySetRIBObject extends BaseRIBObject implements EventListener{
	
	public static final String DIRECTORY_FORWARDING_ENTRY_SET_RIB_OBJECT_NAME = RIBObjectNames.SEPARATOR + 
			RIBObjectNames.DIF + RIBObjectNames.SEPARATOR + RIBObjectNames.MANAGEMENT + RIBObjectNames.SEPARATOR + 
			RIBObjectNames.FLOW_ALLOCATOR + RIBObjectNames.SEPARATOR + RIBObjectNames.DIRECTORY_FORWARDING_TABLE_ENTRIES;

	public static final String DIRECTORY_FORWARDING_TABLE_ENTRY_SET_RIB_OBJECT_CLASS = "directoryforwardingtableentry set";

	public static final String DIRECTORY_FORWARDING_TABLE_ENTRY_RIB_OBJECT_CLASS = "directoryforwardingtableentry";
	
	private static final Log log = LogFactory.getLog(DirectoryForwardingTableEntrySetRIBObject.class);
	
	public DirectoryForwardingTableEntrySetRIBObject(IPCProcess ipcProcess){
		super(ipcProcess, DIRECTORY_FORWARDING_TABLE_ENTRY_SET_RIB_OBJECT_CLASS, 
				ObjectInstanceGenerator.getObjectInstance(), 
				DIRECTORY_FORWARDING_ENTRY_SET_RIB_OBJECT_NAME);
		setRIBDaemon(ipcProcess.getRIBDaemon());
		setEncoder(ipcProcess.getEncoder());
		//Subscribe to events
		this.getRIBDaemon().subscribeToEvent(Event.CONNECTIVITY_TO_NEIGHBOR_LOST, this);
	}
	
	/**
	 * Called by the event manager when one of the events we're subscribed to happens
	 */
	public void eventHappened(Event event) {
		if (event.getId().equals(Event.CONNECTIVITY_TO_NEIGHBOR_LOST)){
			ConnectivityToNeighborLostEvent conEvent = (ConnectivityToNeighborLostEvent) event;
			this.connectivityToNeighborLost(conEvent.getNeighbor());
		}
	}
	
	/**
	 * Called when the connectivity to a neighbor has been lost. All the 
	 * applications registered from that neighbor have to be removed from the directory
	 * @param cdapSessionDescriptor
	 */
	private void connectivityToNeighborLost(Neighbor neighbor){
		List<DirectoryForwardingTableEntry> entriesToDelete = new ArrayList<DirectoryForwardingTableEntry>();
		DirectoryForwardingTableEntry currentEntry = null;

		for(int i=0; i<this.getChildren().size(); i++){
			currentEntry = (DirectoryForwardingTableEntry) this.getChildren().get(i).getObjectValue();
			if (currentEntry.getAddress() == neighbor.getAddress()){
				entriesToDelete.add(currentEntry);
			}
		}
		
		if(entriesToDelete.size() > 0){
			try{
				this.delete(entriesToDelete.toArray(new DirectoryForwardingTableEntry[entriesToDelete.size()]));
			}catch(Exception ex){
				log.error("Problems deleting entries. "+ex.getMessage());
			}
		}
	}
	
	@Override
	/**
	 * A routing update with new and/or updated entries has been received -or during enrollment-. See what parts of the update we didn't now, and 
	 * tell the RIB Daemon about them (will create/update the objects and notify my neighbors except for the one that has 
	 * sent me the update)
	 * @param CDAPMessage the message containing the update
	 * @param CDAPSessionDescriptor the descriptor of the CDAP Session over which I've received the update
	 */
	public void create(CDAPMessage cdapMessage, CDAPSessionDescriptor cdapSessionDescriptor) 
			throws RIBDaemonException{
		if (cdapMessage.getObjValue() == null || cdapMessage.getObjValue().getByteval() == null) {
			return;
		}
		
		DirectoryForwardingTableEntry[] directoryForwardingTableEntries = null;
		List<DirectoryForwardingTableEntry> entriesToCreateOrUpdate = new ArrayList<DirectoryForwardingTableEntry>();
		DirectoryForwardingTableEntry currentEntry = null;
		
		//Decode the object
		try{
			if (cdapMessage.getObjName().equals(DIRECTORY_FORWARDING_ENTRY_SET_RIB_OBJECT_NAME)){
				directoryForwardingTableEntries = (DirectoryForwardingTableEntry[]) 
					this.getEncoder().decode(cdapMessage.getObjValue().getByteval(), DirectoryForwardingTableEntry[].class);
			}else{
				directoryForwardingTableEntries = new DirectoryForwardingTableEntry[]{ (DirectoryForwardingTableEntry)
					this.getEncoder().decode(cdapMessage.getObjValue().getByteval(), DirectoryForwardingTableEntry.class)};
			}
		}catch(Exception ex){
			throw new RIBDaemonException(RIBDaemonException.PROBLEMS_DECODING_OBJECT, ex.getMessage());
		}
		
		
		//Find out what objects have to be updated or created
		for(int i=0; i<directoryForwardingTableEntries.length; i++){
			currentEntry = this.getEntry(directoryForwardingTableEntries[i].getKey());
			
			if (currentEntry == null || !currentEntry.equals(directoryForwardingTableEntries[i])){
				entriesToCreateOrUpdate.add(directoryForwardingTableEntries[i]);
			}
		}
		
		if (entriesToCreateOrUpdate.size() == 0){
			log.debug("No directory forwarding table entries to create or update");
			return;
		}
		
		//Tell the RIB Daemon to create or update the objects, and notify everyone except the neighbor that 
		//has notified me (except if we're in the enrollment phase, then we don't have to notify)
		try{
			NotificationPolicy notificationObject = null;
			
			//Only notify if we're not in the enrollment phase
			//if (getIPCProcess().getOperationalState() == IPCProcess.State.ASSIGNED_TO_DIF){
				notificationObject = new NotificationPolicy(new int[]{cdapSessionDescriptor.getPortId()});
			//}
			
			this.getRIBDaemon().create(DIRECTORY_FORWARDING_TABLE_ENTRY_SET_RIB_OBJECT_CLASS, 
					DIRECTORY_FORWARDING_ENTRY_SET_RIB_OBJECT_NAME,
					entriesToCreateOrUpdate.toArray(new DirectoryForwardingTableEntry[]{}), 
					notificationObject);
		}catch(RIBDaemonException ex){
			log.error(ex.getMessage());
			ex.printStackTrace();
		}
	}
	
	@Override
	/**
	 * One or more local applications have registered to this DIF or a routing update has been received
	 */
	public void create(String objectClass, long objectInstance, String objectName,  Object object) throws RIBDaemonException{
		if (object instanceof DirectoryForwardingTableEntry[]){
			DirectoryForwardingTableEntry[] entries = (DirectoryForwardingTableEntry[]) object;
			for(int i=0; i<entries.length; i++){
				createOrUpdateChildObject(entries[i]);
			}
		}else if (object instanceof DirectoryForwardingTableEntry){
			createOrUpdateChildObject((DirectoryForwardingTableEntry) object);
		}else{
			throw new RIBDaemonException(RIBDaemonException.OBJECTCLASS_DOES_NOT_MATCH_OBJECTNAME, 
					"Object class ("+object.getClass().getName()+") does not match object name "+objectName);
		}
	}
	
	/**
	 * Creates a new child object representing the entry, or updates its value
	 * @param entry
	 */
	private synchronized void createOrUpdateChildObject(DirectoryForwardingTableEntry entry){
		DirectoryForwardingTableEntry existingEntry = this.getEntry(entry.getKey());
		if (existingEntry == null){
			//Create the new object
			RIBObject ribObject = new DirectoryForwardingTableEntryRIBObject(getIPCProcess(),
					this.getObjectName()+RIBObjectNames.SEPARATOR + entry.getKey(), entry);

			//Add it as a child
			this.getChildren().add(ribObject);
			ribObject.setParent(this);

			//Add it to the RIB
			try{
				this.getRIBDaemon().addRIBObject(ribObject);
			}catch(RIBDaemonException ex){
				log.error(ex);
			}
		}else{
			//Update the object value
			existingEntry.setAddress(entry.getAddress());
		}
	}
	
	@Override
	/**
	 * A routing update has been received
	 */
	public void delete(CDAPMessage cdapMessage, CDAPSessionDescriptor cdapSessionDescriptor) throws RIBDaemonException{
		DirectoryForwardingTableEntry[] directoryForwardingTableEntries = null;
		DirectoryForwardingTableEntry currentEntry = null;
		List<DirectoryForwardingTableEntry> entriesToDelete = null;
		
		if (cdapMessage.getObjValue().getByteval() != null){
			//Decode the object
			try{
				if (cdapMessage.getObjName().equals(DIRECTORY_FORWARDING_ENTRY_SET_RIB_OBJECT_NAME)){
					directoryForwardingTableEntries = (DirectoryForwardingTableEntry[]) 
						this.getEncoder().decode(cdapMessage.getObjValue().getByteval(), DirectoryForwardingTableEntry[].class);
				}else{
					directoryForwardingTableEntries = new DirectoryForwardingTableEntry[]{ (DirectoryForwardingTableEntry)
						this.getEncoder().decode(cdapMessage.getObjValue().getByteval(), DirectoryForwardingTableEntry.class)};
				}
			}catch(Exception ex){
				throw new RIBDaemonException(RIBDaemonException.PROBLEMS_DECODING_OBJECT, ex.getMessage());
			}
			
			//Find out what objects have to be deleted
			entriesToDelete = new ArrayList<DirectoryForwardingTableEntry>();
			for(int i=0; i<directoryForwardingTableEntries.length; i++){
				currentEntry = this.getEntry(directoryForwardingTableEntries[i].getKey());
				
				if (currentEntry != null ){
					entriesToDelete.add(directoryForwardingTableEntries[i]);
				}
			}
		}
		
		if(entriesToDelete == null || entriesToDelete.size() == 0){
			log.debug("No directory forwarding table entries to delete");
			return;
		}
		
		//Tell the RIB Daemon to delete the objects, and notify everyone except the neighbor that 
		//has notified me
		try{
			Object objectValue = null;
			if (entriesToDelete != null){
				objectValue = entriesToDelete.toArray(new DirectoryForwardingTableEntry[]{});
			}
			NotificationPolicy notificationObject = new NotificationPolicy(new int[]{cdapSessionDescriptor.getPortId()});
			this.getRIBDaemon().delete(this.getObjectClass(), this.getObjectInstance(), this.getObjectName(),
					objectValue, notificationObject);
		}catch(RIBDaemonException ex){
			log.error(ex.getMessage());
			ex.printStackTrace();
		}
	}
	
	@Override
	/**
	 * One or more local applications have unregistered from this DIF or a routing update has been received
	 */
	public synchronized void delete(Object objectValue) throws RIBDaemonException{
		RIBObject ribObject = null;
		
		if (objectValue == null){
			//All the set has to be deleted
			while(getChildren().size() > 0){
				ribObject = getChildren().remove(0);
				getRIBDaemon().removeRIBObject(ribObject);
			}
		}else if (objectValue instanceof DirectoryForwardingTableEntry[]){
			DirectoryForwardingTableEntry[] entries = (DirectoryForwardingTableEntry[]) objectValue;
			for(int i=0; i<entries.length; i++){
				ribObject = getObject(entries[i].getKey());
				if (ribObject != null){
					getChildren().remove(ribObject);
					getRIBDaemon().removeRIBObject(ribObject);
				}
			}
		}else if (objectValue instanceof DirectoryForwardingTableEntry){
			DirectoryForwardingTableEntry entry = (DirectoryForwardingTableEntry) objectValue;
			ribObject = this.getObject(entry.getKey());
			if (ribObject != null){
				getChildren().remove(ribObject);
				getRIBDaemon().removeRIBObject(ribObject);
			}
		}else{
			throw new RIBDaemonException(RIBDaemonException.OBJECTCLASS_DOES_NOT_MATCH_OBJECTNAME, 
					"Object class ("+objectValue.getClass().getName()+") does not match object name "+this.getObjectName());
		}
	}
	
	/**
	 * Returns the entry identified by the candidateKey attribute
	 * @param candidateKey
	 * @return
	 */
	private DirectoryForwardingTableEntry getEntry(String candidateKey){
		DirectoryForwardingTableEntry currentEntry = null;
		
		for(int i=0; i<this.getChildren().size(); i++){
			currentEntry = (DirectoryForwardingTableEntry) this.getChildren().get(i).getObjectValue();
			if (currentEntry.getApNamingInfo().getEncodedString().equals(candidateKey)){
				return currentEntry;
			}
		}
		
		return null;
	}
	
	/**
	 * Return the child RIB Object whose object value is identified by candidate key
	 * @param candidateKey
	 * @return
	 */
	private RIBObject getObject(String candidateKey){
		RIBObject currentObject = null;
		DirectoryForwardingTableEntry currentEntry = null;
		
		for(int i=0; i<this.getChildren().size(); i++){
			currentObject = this.getChildren().get(i);
			currentEntry = (DirectoryForwardingTableEntry) currentObject.getObjectValue();
			if (currentEntry.getApNamingInfo().getEncodedString().equals(candidateKey)){
				return currentObject;
			}
		}
		
		return null;
	}
	
	
	@Override
	public synchronized Object getObjectValue() {
		DirectoryForwardingTableEntry[] entries = new DirectoryForwardingTableEntry[this.getChildren().size()];
		for(int i=0; i<entries.length; i++){
			entries[i] = (DirectoryForwardingTableEntry) getChildren().get(i).getObjectValue();
		}
		
		return entries;
	}

}
