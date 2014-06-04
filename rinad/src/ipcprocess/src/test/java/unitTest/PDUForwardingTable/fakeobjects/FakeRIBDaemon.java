package unitTest.PDUForwardingTable.fakeobjects;

import java.util.List;
import java.util.concurrent.ConcurrentHashMap;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import eu.irati.librina.QueryRIBRequestEvent;
import rina.cdap.api.CDAPMessageHandler;
import rina.cdap.api.CDAPSessionDescriptor;
import rina.cdap.api.message.CDAPMessage;
import rina.cdap.api.message.ObjectValue;
import rina.encoding.impl.googleprotobuf.flowstate.FlowStateEncoder;
import rina.encoding.impl.googleprotobuf.flowstate.FlowStateGroupEncoder;
import rina.events.api.Event;
import rina.events.api.EventListener;
import rina.ipcprocess.api.IPCProcess;
import rina.pduftg.api.linkstate.FlowStateObject;
import rina.pduftg.api.linkstate.FlowStateObjectGroup;
import rina.ribdaemon.api.NotificationPolicy;
import rina.ribdaemon.api.RIBDaemon;
import rina.ribdaemon.api.RIBDaemonException;
import rina.ribdaemon.api.RIBObject;
import rina.ribdaemon.api.UpdateStrategy;
import rina.encoding.api.Encoder;


public class FakeRIBDaemon implements RIBDaemon{
	
	private static final Log log = LogFactory.getLog(FakeRIBDaemon.class);
	
	public ConcurrentHashMap<String, RIBObject> rib = new ConcurrentHashMap<String, RIBObject>();
	public FlowStateObject recoveredObject = null;
	public FlowStateObjectGroup recoveredObjectGroup = null;
	public boolean waitingResponse = false;

	@Override
	public void subscribeToEvent(String eventId, EventListener eventListener) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void subscribeToEvents(List<String> eventIds,
			EventListener eventListener) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void unsubscribeFromEvent(String eventId, EventListener eventListener) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void unsubscribeFromEvents(List<String> eventIds,
			EventListener eventListener) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void deliverEvent(Event event) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void addRIBObject(RIBObject ribObject) throws RIBDaemonException 
	{
		rib.put(ribObject.getObjectName(), ribObject);
	}

	@Override
	public void removeRIBObject(RIBObject ribObject) throws RIBDaemonException {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void removeRIBObject(String objectName) throws RIBDaemonException {
		if(!rib.containsKey(objectName))
		{
			throw new RIBDaemonException(RIBDaemonException.OBJECTNAME_NOT_PRESENT_IN_THE_RIB);
		}
		rib.remove(objectName);
		
	}

	@Override
	public void sendMessages(CDAPMessage[] cdapMessages,
			UpdateStrategy updateStrategy) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void sendMessage(CDAPMessage cdapMessage, int sessionId,
			CDAPMessageHandler cdapMessageHandler) throws RIBDaemonException {
			try
			{
				switch(cdapMessage.getOpCode())
				{
					case M_WRITE:
							ObjectValue ov = cdapMessage.getObjValue();
							String objectName = cdapMessage.getObjName();
							if (objectName == FlowStateObjectGroup.FLOW_STATE_GROUP_RIB_OBJECT_NAME)
							{
								Encoder encoder = new FlowStateGroupEncoder();
								this.recoveredObjectGroup = (FlowStateObjectGroup) encoder.decode(ov.getByteval(), FlowStateObjectGroup.class);
							}
							else
							{
								Encoder encoder = new FlowStateEncoder();
								this.recoveredObject = (FlowStateObject) encoder.decode(ov.getByteval(), FlowStateObject.class);
							}
						break;
					case M_READ:
						log.info("waiting response entra");
						this.waitingResponse = true;
						break;
					default:
				}
			}
			catch (Exception e) 
			{
				e.printStackTrace();
			}
	}

	@Override
	public void processOperation(CDAPMessage cdapMessage,
			CDAPSessionDescriptor cdapSessionDescriptor)
			throws RIBDaemonException {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void create(String objectClass, long objectInstance,
			String objectName, Object objectValue,
			NotificationPolicy notificationPolicy) throws RIBDaemonException {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void create(String objectClass, String objectName,
			Object objectValue, NotificationPolicy notificationPolicy)
			throws RIBDaemonException {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void create(long objectInstance, Object objectValue,
			NotificationPolicy notificationPolicy) throws RIBDaemonException {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void create(String objectClass, String objectName, Object objectValue)
			throws RIBDaemonException {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void create(long objectInstance, Object objectValue)
			throws RIBDaemonException {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void delete(String objectClass, long objectInstance,
			String objectName, Object objectValue, NotificationPolicy notify)
			throws RIBDaemonException {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void delete(String objectClass, String objectName,
			Object objectValue, NotificationPolicy notificationPolicy)
			throws RIBDaemonException {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void delete(long objectInstance, Object objectValue,
			NotificationPolicy notificationPolicy) throws RIBDaemonException {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void delete(String objectClass, String objectName, Object objectValue)
			throws RIBDaemonException {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void delete(long objectInstance, Object objectValue)
			throws RIBDaemonException {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void delete(String objectClass, long objectInstance,
			String objectName, NotificationPolicy notificationPolicy)
			throws RIBDaemonException {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void delete(String objectClass, long objectInstance,
			String objectName) throws RIBDaemonException {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void delete(String objectClass, String objectName)
			throws RIBDaemonException {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void delete(long objectInstance) throws RIBDaemonException {
		// TODO Auto-generated method stub
		
	}

	@Override
	public RIBObject read(String objectClass, long objectInstance,
			String objectName) throws RIBDaemonException {
		// TODO Auto-generated method stub
		return null;
	}

	@Override
	public RIBObject read(String objectClass, String objectName)
			throws RIBDaemonException {
		// TODO Auto-generated method stub
		return null;
	}

	@Override
	public RIBObject read(long objectInstance) throws RIBDaemonException {
		// TODO Auto-generated method stub
		return null;
	}

	@Override
	public void write(String objectClass, long objectInstance,
			String objectName, Object objectValue, NotificationPolicy notify)
			throws RIBDaemonException {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void write(String objectClass, String objectName,
			Object objectValue, NotificationPolicy notificationPolicy)
			throws RIBDaemonException {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void write(long objectInstance, Object objectValue,
			NotificationPolicy notificationPolicy) throws RIBDaemonException {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void write(String objectClass, String objectName, Object objectValue)
			throws RIBDaemonException {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void write(long objectInstance, Object objectValue)
			throws RIBDaemonException {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void start(String objectClass, long objectInstance,
			String objectName, Object objectValue) throws RIBDaemonException {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void start(String objectClass, String objectName, Object objectValue)
			throws RIBDaemonException {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void start(long objectInstance, Object objectValue)
			throws RIBDaemonException {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void start(String objectClass, String objectName)
			throws RIBDaemonException {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void start(long objectInstance) throws RIBDaemonException {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void stop(String objectClass, long objectInstance,
			String objectName, Object objectValue) throws RIBDaemonException {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void stop(String objectClass, String objectName, Object objectValue)
			throws RIBDaemonException {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void stop(long objectInstance, Object objectValue)
			throws RIBDaemonException {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void processQueryRIBRequestEvent(QueryRIBRequestEvent event) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public List<RIBObject> getRIBObjects() {
		// TODO Auto-generated method stub
		return null;
	}

	@Override
	public void setIPCProcess(IPCProcess ipcProcess) {
		// TODO Auto-generated method stub
		
	}
	
	@Override
	public void sendMessageToAddress(CDAPMessage arg0, int arg1, long arg2,
			CDAPMessageHandler arg3) throws RIBDaemonException {
		// TODO Auto-generated method stub
		
	}

}
