package unitTest.PDUForwardingTable.timertasks;


import java.util.Timer;

import junit.framework.Assert;

import org.junit.Before;
import org.junit.Test;

import rina.encoding.impl.googleprotobuf.flowstate.FlowStateEncoder;
import rina.ipcprocess.api.IPCProcess;
import rina.ipcprocess.impl.PDUForwardingTable.PDUFTImpl;
import rina.ipcprocess.impl.PDUForwardingTable.routingalgorithms.dijkstra.DijkstraAlgorithm;
import rina.ipcprocess.impl.PDUForwardingTable.routingalgorithms.dijkstra.Vertex;
import rina.ipcprocess.impl.PDUForwardingTable.timertasks.PropagateFSODB;
import unitTest.PDUForwardingTable.fakeobjects.FakeCDAPSessionManager;
import unitTest.PDUForwardingTable.fakeobjects.FakeIPCProcess;
import unitTest.PDUForwardingTable.fakeobjects.FakeRIBDaemon;

public class PropagateFSODBTest {

	protected PDUFTImpl impl = null;
	
	static {
		System.loadLibrary("rina_java");
	}
	
	@Before
	public void set()
	{
		impl = new PDUFTImpl(2147483647);
	}
	
	@Test
	public void Run_SendObject_NotNULL() {
		Timer timer = new Timer();
		FakeRIBDaemon rib = new FakeRIBDaemon();
		IPCProcess ipc = new FakeIPCProcess(new FakeCDAPSessionManager(), rib, new FlowStateEncoder());
		impl.setIPCProcess(ipc);
		impl.setAlgorithm(new DijkstraAlgorithm(), new Vertex(1));
		
		impl.flowAllocated(1, 1, 2, 1);
		timer.schedule(new PropagateFSODB(impl), 1);
		try {
		    Thread.sleep(500);
		} catch(InterruptedException ex) {
		    Thread.currentThread().interrupt();
		}
		
		Assert.assertNotNull(rib.recoveredObject);
	}

	//@Test
	public void Run_SendObjectSameObject_True() {
		Timer timer = new Timer();
		FakeRIBDaemon rib = new FakeRIBDaemon();
		IPCProcess ipc = new FakeIPCProcess(new FakeCDAPSessionManager(), rib, new FlowStateEncoder());
		impl.setIPCProcess(ipc);
		impl.setAlgorithm(new DijkstraAlgorithm(), new Vertex(1));
		
		impl.flowAllocated(1, 1, 2, 1);
		timer.schedule(new PropagateFSODB(impl), 1);
		try {
		    Thread.sleep(500);
		} catch(InterruptedException ex) {
		    Thread.currentThread().interrupt();
		}
		
		Assert.assertEquals(rib.recoveredObject.getAddress(), 1);
		Assert.assertEquals(rib.recoveredObject.getPortid(), 1);
		Assert.assertEquals(rib.recoveredObject.getNeighborAddress(), 2);
		Assert.assertEquals(rib.recoveredObject.getNeighborPortid(), 1);
	}
}
