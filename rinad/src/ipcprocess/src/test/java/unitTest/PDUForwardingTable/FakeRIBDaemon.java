package unitTest.PDUForwardingTable;

import java.util.List;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import eu.irati.librina.QueryRIBRequestEvent;
import rina.PDUForwardingTable.api.FlowStateObject;
import rina.PDUForwardingTable.api.FlowStateObjectGroup;
import rina.cdap.api.CDAPMessageHandler;
import rina.cdap.api.CDAPSessionDescriptor;
import rina.cdap.api.message.CDAPMessage;
import rina.cdap.api.message.ObjectValue;
import rina.encoding.impl.googleprotobuf.flowstate.FlowStateEncoder;
import rina.encoding.impl.googleprotobuf.flowstate.FlowStateGroupEncoder;
import rina.events.api.Event;
import rina.events.api.EventListener;
import rina.ipcprocess.api.IPCProcess;
import rina.ipcprocess.impl.IPCProcessImpl;
import rina.ribdaemon.api.NotificationPolicy;
import rina.ribdaemon.api.RIBDaemon;
import rina.ribdaemon.api.RIBDaemonException;
import rina.ribdaemon.api.RIBObject;
import rina.ribdaemon.api.UpdateStrategy;
import rina.encoding.api.Encoder;
import rina.encoding.impl.googleprotobuf.flowstate.FlowStateEncoder;


public class FakeRIBDaemon implements RIBDaemon{
	
	private static final Log log = LogFactory.getLog(FakeRIBDaemon.class);
	
	public FlowStateObjectGroup recoveredObject = null;

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
	public void addRIBObject(RIBObject ribObject) throws RIBDaemonException {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void removeRIBObject(RIBObject ribObject) throws RIBDaemonException {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void removeRIBObject(String objectName) throws RIBDaemonException {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void sendMessages(CDAPMessage[] cdapMessages,
			UpdateStrategy updateStrategy) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void sendMessage(CDAPMessage cdapMessage, int sessionId,
			CDAPMessageHandler cdapMessageHandler) throws RIBDaemonException {
			Encoder encoder = new FlowStateGroupEncoder();
			
			ObjectValue ov = cdapMessage.getObjValue();
			try {
				this.recoveredObject = (FlowStateObjectGroup) encoder.decode(ov.getByteval(), FlowStateObjectGroup.class);
			} catch (Exception e) {
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

}
