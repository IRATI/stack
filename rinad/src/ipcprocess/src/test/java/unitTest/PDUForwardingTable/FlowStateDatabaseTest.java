package unitTest.PDUForwardingTable;

import junit.framework.Assert;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.junit.Before;
import org.junit.Test;

import rina.encoding.impl.googleprotobuf.flowstate.FlowStateGroupEncoder;
import rina.ipcprocess.api.IPCProcess;
import rina.ipcprocess.impl.PDUForwardingTable.FlowStateDatabase;
import rina.ipcprocess.impl.PDUForwardingTable.PDUFTImpl;
import rina.ipcprocess.impl.PDUForwardingTable.internalobjects.FlowStateInternalObject;
import rina.ipcprocess.impl.PDUForwardingTable.internalobjects.FlowStateInternalObjectGroup;
import rina.ipcprocess.impl.PDUForwardingTable.ribobjects.FlowStateRIBObjectGroup;
import unitTest.PDUForwardingTable.fakeobjects.FakeCDAPSessionManager;
import unitTest.PDUForwardingTable.fakeobjects.FakeIPCProcess;
import unitTest.PDUForwardingTable.fakeobjects.FakeRIBDaemon;

public class FlowStateDatabaseTest {

	private static final Log log = LogFactory.getLog(FlowStateDatabaseTest.class);
	FlowStateInternalObject obj1 = null;
	FlowStateInternalObject obj2 = null;
	FlowStateInternalObject obj3 = null;
	FlowStateInternalObject obj4 = null;
	
	static {
		System.loadLibrary("rina_java");
	}
	
	@Before
	public void set()
	{
		obj1 = new FlowStateInternalObject(1,1,2,1, true, 1, 0);
		obj2 = new FlowStateInternalObject(2,1,1,1, true, 1, 0);
		obj3 = new FlowStateInternalObject(1,2,3,2, true, 1, 0);
		obj4 = new FlowStateInternalObject(3,2,1,2, true, 1, 0);
	}
	
	@Test
	public void addObjectToGroup_NoObjectCheckModified_False()
	{
		FlowStateDatabase db = new FlowStateDatabase();
		
		Assert.assertFalse(db.isModified());
	}

	@Test
	public void addObjectToGroup_AddObjectCheckModified_True()
	{
		FlowStateDatabase db = new FlowStateDatabase();
		FakeRIBDaemon rib = new FakeRIBDaemon();
		IPCProcess ipc = new FakeIPCProcess(new FakeCDAPSessionManager(), rib, new FlowStateGroupEncoder());
		
		db.addObjectToGroup(obj1, new FlowStateRIBObjectGroup(new PDUFTImpl(5000),ipc));
		
		Assert.assertTrue(db.isModified());
	}
	
	@Test
	public void incrementAge_AddObjectCheckModified_False()
	{
		FlowStateDatabase db = new FlowStateDatabase();
		FakeRIBDaemon rib = new FakeRIBDaemon();
		IPCProcess ipc = new FakeIPCProcess(new FakeCDAPSessionManager(), rib, new FlowStateGroupEncoder());
		
		db.addObjectToGroup(obj1, new FlowStateRIBObjectGroup(new PDUFTImpl(5000),ipc));
		db.setModified(false);
		db.incrementAge(3);
		
		Assert.assertFalse(db.isModified());
	}
	
	@Test
	public void incrementAge_AddObjectCheckModified_True()
	{
		FlowStateDatabase db = new FlowStateDatabase();
		FakeRIBDaemon rib = new FakeRIBDaemon();
		IPCProcess ipc = new FakeIPCProcess(new FakeCDAPSessionManager(), rib, new FlowStateGroupEncoder());
		
		db.addObjectToGroup(obj1, new FlowStateRIBObjectGroup(new PDUFTImpl(5000),ipc));
		db.setModified(false);
		
		db.incrementAge(1);
		
		Assert.assertTrue(db.isModified());
	}
	/*
	@Test
	public void updateObjects_NewObjectIsModified_True()
	{
		FlowStateDatabase db = new FlowStateDatabase();
		FlowStateInternalObjectGroup fsg = new FlowStateInternalObjectGroup();
		fsg.add(obj1);
		
		db.updateObjects(fsg, 2);
		
		Assert.assertTrue(db.isModified());
	}

	@Test
	public void updateObjects_NewObjectAddedSize_True()
	{
		FlowStateDatabase db = new FlowStateDatabase();
		FlowStateInternalObjectGroup fsg = new FlowStateInternalObjectGroup();
		fsg.add(obj1);
		fsg.add(obj2);
		
		db.updateObjects(fsg, 2);
		
		Assert.assertEquals(db.getFlowStateObjectGroup().getFlowStateObjectArray().size(), 2);
	}
	
	@Test
	public void updateObjects_NewObjectExist_True()
	{
		FlowStateDatabase db = new FlowStateDatabase();
		FlowStateInternalObjectGroup fsg = new FlowStateInternalObjectGroup();
		fsg.add(obj1);
		
		db.updateObjects(fsg, 2);
		
		Assert.assertEquals(db.getFlowStateObjectGroup().getFlowStateObjectArray().get(0), obj1);
	}

	@Test
	public void updateObjects_ObjectExistIsModified_True()
	{
		FlowStateDatabase db = new FlowStateDatabase();
		FlowStateInternalObjectGroup fsg = new FlowStateInternalObjectGroup();
		FakeRIBDaemon rib = new FakeRIBDaemon();
		IPCProcess ipc = new FakeIPCProcess(new FakeCDAPSessionManager(), rib, new FlowStateGroupEncoder());
		db.addObjectToGroup(obj1, new FlowStateRIBObjectGroup(new PDUFTImpl(5000),ipc));
		
		FlowStateInternalObject tempObj = new FlowStateInternalObject(1,1,2,1, true, 2, 0);
		fsg.add(tempObj);
		db.updateObjects(fsg, 2);
		
		Assert.assertTrue(db.isModified());
	}
	
	@Test
	public void updateObjects_ObjectExistUpdate_True()
	{
		FlowStateDatabase db = new FlowStateDatabase();
		FlowStateInternalObjectGroup fsg = new FlowStateInternalObjectGroup();
		FakeRIBDaemon rib = new FakeRIBDaemon();
		IPCProcess ipc = new FakeIPCProcess(new FakeCDAPSessionManager(), rib, new FlowStateGroupEncoder());
		db.addObjectToGroup(obj1, new FlowStateRIBObjectGroup(new PDUFTImpl(5000),ipc));
		
		FlowStateInternalObject tempObj = new FlowStateInternalObject(1,1,2,1, true, 2, 0);
		fsg.add(tempObj);
		db.updateObjects(fsg, 2);
		
		Assert.assertEquals(db.getFlowStateObjectGroup().getFlowStateObjectArray().get(0).getSequenceNumber(), 2);
	}
	
	
	@Test
	public void updateObjects_ObjectDoesNotExistNoUpdate_False()
	{
		FlowStateDatabase db = new FlowStateDatabase();
		FlowStateInternalObjectGroup fsg = new FlowStateInternalObjectGroup();
		FakeRIBDaemon rib = new FakeRIBDaemon();
		IPCProcess ipc = new FakeIPCProcess(new FakeCDAPSessionManager(), rib, new FlowStateGroupEncoder());
		db.addObjectToGroup(obj1, new FlowStateRIBObjectGroup(new PDUFTImpl(5000),ipc));
		
		FlowStateInternalObject tempObj = new FlowStateInternalObject(1,1,3,2, true, 2, 0);
		fsg.add(tempObj);
		db.updateObjects(fsg, 2);
		
		Assert.assertNotSame(db.getFlowStateObjectGroup().getFlowStateObjectArray().get(0).getSequenceNumber(), 2);
	}
	
	@Test
	public void updateObjects_NoSeqNumberGreaterNoUpdate_True()
	{
		FlowStateDatabase db = new FlowStateDatabase();
		FlowStateInternalObjectGroup fsg = new FlowStateInternalObjectGroup();
		FakeRIBDaemon rib = new FakeRIBDaemon();
		IPCProcess ipc = new FakeIPCProcess(new FakeCDAPSessionManager(), rib, new FlowStateGroupEncoder());
		db.addObjectToGroup(obj1, new FlowStateRIBObjectGroup(new PDUFTImpl(5000),ipc));
		
		FlowStateInternalObject tempObj = new FlowStateInternalObject(1,1,2,1, true, 0, 0);
		fsg.add(tempObj);
		db.updateObjects(fsg, 2);
		
		Assert.assertNotSame(db.getFlowStateObjectGroup().getFlowStateObjectArray().get(0).getSequenceNumber(), 0);
	}
	*/
}
