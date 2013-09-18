package rina.ribdaemon.api;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import rina.events.api.Event;
import rina.events.api.EventListener;
import rina.ipcprocess.api.BaseIPCProcessComponent;

/**
 * Provides the component name for the RIB Daemon, and implements the basic 
 * RIB Daemon and event Manager behavior
 * @author eduardgrasa
 *
 */
public abstract class BaseRIBDaemon extends BaseIPCProcessComponent implements RIBDaemon{
	
	private static final Log log = LogFactory.getLog(BaseRIBDaemon.class);
	
	protected Map<String, List<EventListener>> eventListeners = null;
	
	public BaseRIBDaemon(){
		this.eventListeners = new HashMap<String, List<EventListener>>();
	}
	
	/**
	 * Subscribe to a single event
	 * @param eventId The id of the event
	 * @param eventListener The event listener
	 */
	public void subscribeToEvent(String eventId, EventListener eventListener){
		if (eventId == null || eventListener == null){
			return;
		}
		
		List<EventListener> listeners = null;
		synchronized(this.eventListeners){
			if(this.eventListeners.containsKey(eventId)){
				listeners = this.eventListeners.get(eventId);
				if(!listeners.contains(eventListener)){
					listeners.add(eventListener);
				}
			}else{
				listeners = new ArrayList<EventListener>();
				listeners.add(eventListener);
				this.eventListeners.put(eventId, listeners);
			}
			
			log.info("EventListener "+eventListener.toString()+
					" subscribed to event "+eventId);
		}
	}
	
	/**
	 * Subscribes to a list of events
	 * @param eventIds the list of event ids
	 * @param eventListener The event listener
	 */
	public void subscribeToEvents(List<String> eventIds, EventListener eventListener){
		if (eventIds == null || eventListener == null){
			return;
		}
		
		for(int i=0; i<eventIds.size(); i++){
			this.subscribeToEvent(eventIds.get(i), eventListener);
		}
	}
	
	/**
	 * Unubscribe from a single event
	 * @param eventId The id of the event
	 * @param eventListener The event listener
	 */
	public void unsubscribeFromEvent(String eventId, EventListener eventListener){
		if (eventId == null || eventListener == null){
			return;
		}
		
		List<EventListener> listeners = null;
		synchronized(this.eventListeners){
			if(this.eventListeners.containsKey(eventId)){
				listeners = this.eventListeners.get(eventId);
				if (listeners.contains(eventListener)){
					listeners.remove(eventListener);
					log.info("EventListener "+eventListener.toString()+
							" unsubscribed from event "+eventId);
					if (listeners.size() == 0){
						this.eventListeners.remove(eventId);
					}
				}
			}
		}
	}
	
	/**
	 * Unsubscribe from a list of events
	 * @param eventIds the list of event ids
	 * @param eventListener The event listener
	 */
	public void unsubscribeFromEvents(List<String> eventIds, EventListener eventListener){
		if (eventIds == null || eventListener == null){
			return;
		}
		
		for(int i=0; i<eventIds.size(); i++){
			this.unsubscribeFromEvent(eventIds.get(i), eventListener);
		}
	}
	
	/**
	 * Invoked when a certain event has happened
	 * @param event
	 */
	public void deliverEvent(Event event){
		log.info("Event "+event.getId()+" has just happened. Notifying event listeners.");
		
		List<EventListener> listeners = null;
		synchronized(this.eventListeners){
			listeners = this.eventListeners.get(event.getId());
		}
		
		if (listeners == null){
			return;
		}
		
		for(int i=0; i<listeners.size(); i++){
			listeners.get(i).eventHappened(event);
		}
	}

	public static final String getComponentName(){
		return RIBDaemon.class.getName();
	}
	
	public String getName() {
		return BaseRIBDaemon.getComponentName();
	}

	/**
	 * Create or update an object in the RIB
	 * @param objectClass the class of the object
	 * @param objectName the name of the object
	 * @param objectInstance the instance of the object
	 * @param objectValue the value of the object
	 * @param notify if not null notify some of the neighbors about the change
	 * @throws RIBDaemonException
	 */
	public abstract void create(String objectClass, long objectInstance, String objectName, Object objectValue, 
			NotificationPolicy notificationPolicy) throws RIBDaemonException;
	
	public void create(String objectClass, String objectName, Object objectValue, 
			NotificationPolicy notificationPolicy) throws RIBDaemonException{
		this.create(objectClass, 0L, objectName, objectValue, notificationPolicy);
	}

	public void create(long objectInstance, Object objectValue, NotificationPolicy notificationPolicy) throws RIBDaemonException{
		this.create(null, objectInstance, null, objectValue, notificationPolicy);
	}
	
	public void create(String objectClass, String objectName, Object objectValue) throws RIBDaemonException{
		this.create(objectClass, 0L, objectName, objectValue, null);
	}
	
	public void create(long objectInstance, Object objectValue) throws RIBDaemonException{
		this.create(null, objectInstance, null, objectValue, null);
	}
	
	/**
	 * Delete an object from the RIB
	 * @param objectClass the class of the object
	 * @param objectName the name of the object
	 * @param objectInstance the instance of the object
	 * @param object the value of the object
	 * @param notify if not null notify some of the neighbors about the change
	 * @throws RIBDaemonException 
	 */
	public abstract void delete(String objectClass, long objectInstance, String objectName, Object objectValue, 
			NotificationPolicy notificationPolicy) throws RIBDaemonException;
	
	public void delete(String objectClass, String objectName, Object objectValue, 
			NotificationPolicy notificationPolicy) throws RIBDaemonException{
		this.delete(objectClass, 0L, objectName, objectValue, notificationPolicy);
	}

	public void delete(long objectInstance, Object objectValue, NotificationPolicy notificationPolicy) throws RIBDaemonException{
		this.delete(null, objectInstance, null, objectValue, notificationPolicy);
	}
	
	public void delete(String objectClass, String objectName, Object objectValue) throws RIBDaemonException{
		this.delete(objectClass, 0L, objectName, objectValue, null);
	}
	
	public void delete(long objectInstance, Object objectValue) throws RIBDaemonException{
		this.delete(null, objectInstance, null, objectValue, null);
	}
	
	public void delete(String objectClass, long objectInstance, String objectName, NotificationPolicy notificationPolicy) throws RIBDaemonException{
		this.delete(objectClass, objectInstance, objectName, null, notificationPolicy);
	}
	
	public void delete(String objectClass, long objectInstance, String objectName) throws RIBDaemonException{
		this.delete(objectClass, objectInstance, objectName, null, null);
	}
	
	public void delete(String objectClass, String objectName) throws RIBDaemonException{
		this.delete(objectClass, 0L, objectName, null, null);
	}
	
	public void delete(long objectInstance) throws RIBDaemonException{
		this.delete(null, objectInstance, null, null, null);
	}
	
	/**
	 * Read an object from the RIB
	 * @param objectClass the class of the object
	 * @param objectName the name of the object
	 * @param objectInstance the instance of the object
	 * @return
	 * @throws RIBDaemonException
	 */
	public abstract RIBObject read(String objectClass, long objectInstance, String objectName) throws RIBDaemonException;
	
	public RIBObject read(String objectClass, String objectName) throws RIBDaemonException{
		return this.read(objectClass, 0L, objectName);
	}
	
	public RIBObject read(long objectInstance) throws RIBDaemonException{
		return this.read(null, objectInstance, null);
	}
	
	/**
	 * Update the value of an object in the RIB
	 * @param objectClass the class of the object
	 * @param objectName the name of the object
	 * @param objectInstance the instance of the object
	 * @param objectValue the new value of the object
	 * @param notify if not null notify some of the neighbors about the change
	 * @throws RIBDaemonException
	 */
	public abstract void write(String objectClass, long objectInstance, String objectName, Object objectValue, 
			NotificationPolicy notificationPolicy) throws RIBDaemonException;
	
	public void write(String objectClass, String objectName, Object objectValue, 
			NotificationPolicy notificationPolicy) throws RIBDaemonException{
		this.write(objectClass, 0L, objectName, objectValue, notificationPolicy);
	}

	public void write(long objectInstance, Object objectValue, NotificationPolicy notificationPolicy) throws RIBDaemonException{
		this.write(null, objectInstance, null, objectValue, notificationPolicy);
	}
	
	public void write(String objectClass, String objectName, Object objectValue) throws RIBDaemonException{
		this.write(objectClass, 0L, objectName, objectValue, null);
	}
	
	public void write(long objectInstance, Object objectValue) throws RIBDaemonException{
		this.write(null, objectInstance, null, objectValue, null);
	}
	
	/**
	 * Start an object from the RIB
	 * @param objectClass the class of the object
	 * @param objectName the name of the object
	 * @param objectInstance the instance of the object
	 * @param objectValue the new value of the object
	 * @throws RIBDaemonException
	 */
	public abstract void start(String objectClass, long objectInstance, String objectName, Object objectValue) throws RIBDaemonException;
	
	public void start(String objectClass, String objectName, Object objectValue) throws RIBDaemonException{
		this.start(objectClass, 0L, objectName, objectValue);
	}
	
	public void start(long objectInstance, Object objectValue) throws RIBDaemonException{
		this.start(null, objectInstance, null, objectValue);
	}
	
	public void start(String objectClass, String objectName) throws RIBDaemonException{
		this.start(objectClass, 0L, objectName, null);
	}
	
	public void start(long objectInstance) throws RIBDaemonException{
		this.start(null, objectInstance, null, null);
	}
	
	/**
	 * Stop an object from the RIB
	 * @param objectClass the class of the object
	 * @param objectName the name of the object
	 * @param objectInstance the instance of the object
	 * @param objectValue the new value of the object
	 * @throws RIBDaemonException
	 */
	public abstract void stop(String objectClass, long objectInstance, String objectName, Object objectValue) throws RIBDaemonException;
	
	public void stop(String objectClass, String objectName, Object objectValue) throws RIBDaemonException{
		this.stop(objectClass, 0L, objectName, objectValue);
	}
	
	public void stop(long objectInstance, Object objectValue) throws RIBDaemonException{
		this.stop(null, objectInstance, null, objectValue);
	}
}
