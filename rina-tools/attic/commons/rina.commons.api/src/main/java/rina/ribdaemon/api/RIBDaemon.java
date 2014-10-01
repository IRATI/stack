package rina.ribdaemon.api;

import java.util.List;

import eu.irati.librina.QueryRIBRequestEvent;

import rina.cdap.api.CDAPMessageHandler;
import rina.cdap.api.CDAPSessionDescriptor;
import rina.cdap.api.message.CDAPMessage;
import rina.events.api.EventManager;
import rina.ipcprocess.api.IPCProcessComponent;

/**
 * Specifies the interface of the RIB Daemon
 * @author eduardgrasa
 */
public interface RIBDaemon extends EventManager, IPCProcessComponent {
	
	/**
	 * Add a RIB object to the RIB
	 * @param ribHandler
	 * @param objectName
	 * @throws RIBDaemonException
	 */
	public void addRIBObject(RIBObject ribObject) throws RIBDaemonException;
	
	/**
	 * Remove a ribObject from the RIB
	 * @param ribObject
	 * @throws RIBDaemonException
	 */
	public void removeRIBObject(RIBObject ribObject) throws RIBDaemonException;
	
	/**
	 * Remove a ribObject from the RIB
	 * @param objectName
	 * @throws RIBDaemonException
	 */
	public void removeRIBObject(String objectName) throws RIBDaemonException;
	
	/**
	 * Send an information update, consisting on a set of CDAP messages, using the updateStrategy update strategy
	 * (on demand, scheduled)
	 * @param cdapMessages
	 * @param updateStrategy
	 */
	public void sendMessages(CDAPMessage[] cdapMessages, UpdateStrategy updateStrategy);
	
	/**
	 * Causes a CDAP message to be sent
	 * @param cdapMessage the message to be sent
	 * @param sessionId the CDAP session id
	 * @param cdapMessageHandler the class to be called when the response message is received (if required)
	 * @throws RIBDaemonException
	 */
	public void sendMessage(CDAPMessage cdapMessage, int sessionId,
			CDAPMessageHandler cdapMessageHandler) throws RIBDaemonException;
	
	/**
	 * Causes a CDAP message to be sent
	 * @param cdapMessage the message to be sent
	 * @param sessionId the CDAP session id
	 * @param address the address of the IPC Process to send the Message To
	 * @param cdapMessageHandler the class to be called when the response message is received (if required)
	 * @throws RIBDaemonException
	 */
	public void sendMessageToAddress(CDAPMessage cdapMessage, int sessionId, long address,
			CDAPMessageHandler cdapMessageHandler) throws RIBDaemonException;
	
	/**
	 * Reads/writes/created/deletes/starts/stops one or more objects at the RIB, matching the information specified by objectId + objectClass or objectInstance.
	 * At least objectName or objectInstance have to be not null. This operation is invoked because the RIB Daemon has received a CDAP message from another 
	 * IPC process
	 * @param cdapMessage The CDAP message received
	 * @param cdapSessionDescriptor Describes the CDAP session to where the CDAP message belongs
	 * @throws RIBDaemonException on a number of circumstances
	 */
	public void processOperation(CDAPMessage cdapMessage, CDAPSessionDescriptor cdapSessionDescriptor) throws RIBDaemonException;
	
	/**
	 * Create or update an object in the RIB
	 * @param objectClass the class of the object
	 * @param objectName the name of the object
	 * @param objectInstance the instance of the object
	 * @param objectValue the value of the object
	 * @param notify if not null notify some of the neighbors about the change
	 * @throws RIBDaemonException
	 */
	public void create(String objectClass, long objectInstance, String objectName, Object objectValue, NotificationPolicy notificationPolicy) throws RIBDaemonException;
	public void create(String objectClass, String objectName, Object objectValue, NotificationPolicy notificationPolicy) throws RIBDaemonException;
	public void create(long objectInstance, Object objectValue, NotificationPolicy notificationPolicy) throws RIBDaemonException;
	public void create(String objectClass, String objectName, Object objectValue) throws RIBDaemonException;
	public void create(long objectInstance, Object objectValue) throws RIBDaemonException;
	
	/**
	 * Delete an object from the RIB
	 * @param objectClass the class of the object
	 * @param objectName the name of the object
	 * @param objectInstance the instance of the object
	 * @param object the value of the object
	 * @param notify if not null notify some of the neighbors about the change
	 * @throws RIBDaemonException 
	 */
	public void delete(String objectClass, long objectInstance, String objectName, Object objectValue, NotificationPolicy notify) throws RIBDaemonException;
	public void delete(String objectClass, String objectName, Object objectValue, NotificationPolicy notificationPolicy) throws RIBDaemonException;
	public void delete(long objectInstance, Object objectValue, NotificationPolicy notificationPolicy) throws RIBDaemonException;
	public void delete(String objectClass, String objectName, Object objectValue) throws RIBDaemonException;
	public void delete(long objectInstance, Object objectValue) throws RIBDaemonException;
	public void delete(String objectClass, long objectInstance, String objectName, NotificationPolicy notificationPolicy) throws RIBDaemonException;
	public void delete(String objectClass, long objectInstance, String objectName) throws RIBDaemonException;
	public void delete(String objectClass, String objectName) throws RIBDaemonException;
	public void delete(long objectInstance) throws RIBDaemonException;
	
	/**
	 * Read an object from the RIB
	 * @param objectClass the class of the object
	 * @param objectName the name of the object
	 * @param objectInstance the instance of the object
	 * @return
	 * @throws RIBDaemonException
	 */
	public RIBObject read(String objectClass, long objectInstance, String objectName) throws RIBDaemonException;
	public RIBObject read(String objectClass, String objectName) throws RIBDaemonException;
	public RIBObject read(long objectInstance) throws RIBDaemonException;
	
	/**
	 * Update the value of an object in the RIB
	 * @param objectClass the class of the object
	 * @param objectName the name of the object
	 * @param objectInstance the instance of the object
	 * @param objectValue the new value of the object
	 * @param notify if not null notify some of the neighbors about the change
	 * @throws RIBDaemonException
	 */
	public void write(String objectClass, long objectInstance, String objectName, Object objectValue, NotificationPolicy notify) throws RIBDaemonException;
	public void write(String objectClass, String objectName, Object objectValue, NotificationPolicy notificationPolicy) throws RIBDaemonException;
	public void write(long objectInstance, Object objectValue, NotificationPolicy notificationPolicy) throws RIBDaemonException;
	public void write(String objectClass, String objectName, Object objectValue) throws RIBDaemonException;
	public void write(long objectInstance, Object objectValue) throws RIBDaemonException;
	
	/**
	 * Start an object at the RIB
	 * @param objectClass the class of the object
	 * @param objectName the name of the object
	 * @param objectInstance the instance of the object
	 * @param objectValue the new value of the object
	 * @throws RIBDaemonException
	 */
	public void start(String objectClass, long objectInstance, String objectName, Object objectValue) throws RIBDaemonException;
	public void start(String objectClass, String objectName, Object objectValue) throws RIBDaemonException;
	public void start(long objectInstance, Object objectValue) throws RIBDaemonException;
	public void start(String objectClass, String objectName) throws RIBDaemonException;
	public void start(long objectInstance) throws RIBDaemonException;
	
	/**
	 * Stop an object at the RIB
	 * @param objectClass the class of the object
	 * @param objectName the name of the object
	 * @param objectInstance the instance of the object
	 * @param objectValue the new value of the object
	 * @throws RIBDaemonException
	 */
	public void stop(String objectClass, long objectInstance, String objectName, Object objectValue) throws RIBDaemonException;
	public void stop(String objectClass, String objectName, Object objectValue) throws RIBDaemonException;
	public void stop(long objectInstance, Object objectValue) throws RIBDaemonException;
	
	/**
	 * Process a Query RIB Request from the IPC Manager
	 * @param event
	 */
	public void processQueryRIBRequestEvent(QueryRIBRequestEvent event);
	
	public List<RIBObject> getRIBObjects();
}