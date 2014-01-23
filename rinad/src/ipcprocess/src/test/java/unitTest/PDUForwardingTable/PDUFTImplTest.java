package unitTest.PDUForwardingTable;


import junit.framework.Assert;

import org.junit.Test;

import eu.irati.librina.ApplicationProcessNamingInformation;
import rina.encoding.impl.googleprotobuf.flowstate.FlowStateEncoder;
import rina.encoding.impl.googleprotobuf.flowstate.FlowStateGroupEncoder;
import rina.ipcprocess.api.IPCProcess;
import rina.ipcprocess.impl.PDUForwardingTable.PDUFTImpl;
import rina.ipcprocess.impl.PDUForwardingTable.routingalgorithms.dijkstra.DijkstraAlgorithm;
import rina.ipcprocess.impl.PDUForwardingTable.routingalgorithms.dijkstra.Vertex;

public class PDUFTImplTest {

	@Test
	public void PDUFTImpl_Constructor_True()
	{
		PDUFTImpl implementation = new PDUFTImpl();
		Assert.assertEquals(implementation.getClass(), PDUFTImpl.class);
	}
	
	@Test
	public void flowAllocated_AllocateFlow_True()
	{
		PDUFTImpl impl = new PDUFTImpl();
		impl.setIPCProcess(new FakeIPCProcess(new FakeCDAPSessionManager(), new FakeRIBDaemon(), new FlowStateEncoder()));
		impl.setAlgorithm(new DijkstraAlgorithm(), new Vertex(1));
		
		Assert.assertTrue(impl.flowAllocated(1, 1, 2, 1));
	}
	
	@Test
	public void flowdeAllocated_DeAllocateFlow_True()
	{
		PDUFTImpl impl = new PDUFTImpl();
		impl.setIPCProcess(new FakeIPCProcess(new FakeCDAPSessionManager(), new FakeRIBDaemon(), new FlowStateEncoder()));
		impl.setAlgorithm(new DijkstraAlgorithm(), new Vertex(1));
		
		impl.flowAllocated(1, 1, 2, 1);
		
		Assert.assertTrue(impl.flowdeAllocated(1));
	}

	@Test
	public void flowdeAllocated_DeAllocateFlow_False()
	{
		PDUFTImpl impl = new PDUFTImpl();
		impl.setIPCProcess(new FakeIPCProcess(new FakeCDAPSessionManager(), new FakeRIBDaemon(), new FlowStateEncoder()));
		impl.setAlgorithm(new DijkstraAlgorithm(), new Vertex(1));
		
		impl.flowAllocated(1, 1, 2, 1);
		
		Assert.assertFalse(impl.flowdeAllocated(2));
	}
	
	@Test
	public void enrollmentToNeighbor_SendMessage_NotNull()
	{
		PDUFTImpl impl = new PDUFTImpl();
		FakeRIBDaemon rib = new FakeRIBDaemon();
		IPCProcess ipc = new FakeIPCProcess(new FakeCDAPSessionManager(), rib, new FlowStateGroupEncoder());
		impl.setIPCProcess(ipc);
		impl.setAlgorithm(new DijkstraAlgorithm(), new Vertex(1));
		
		impl.flowAllocated(1, 1, 2, 1);
		impl.enrollmentToNeighbor(new ApplicationProcessNamingInformation(), 3, true, 2);
		
		Assert.assertNotNull(rib.recoveredObject);
	}
	
	@Test
	public void enrollmentToNeighbor_NoFlowAllocateds_Null()
	{
		PDUFTImpl impl = new PDUFTImpl();
		FakeRIBDaemon rib = new FakeRIBDaemon();
		IPCProcess ipc = new FakeIPCProcess(new FakeCDAPSessionManager(), rib, new FlowStateGroupEncoder());
		impl.setIPCProcess(ipc);
		impl.setAlgorithm(new DijkstraAlgorithm(), new Vertex(1));
		
		impl.enrollmentToNeighbor(new ApplicationProcessNamingInformation(), 3, true, 2);
		
		Assert.assertNull(rib.recoveredObject);
	}
	
	@Test
	public void enrollmentToNeighbor_SendMessageRecoverObject_True()
	{
		PDUFTImpl impl = new PDUFTImpl();
		FakeRIBDaemon rib = new FakeRIBDaemon();
		IPCProcess ipc = new FakeIPCProcess(new FakeCDAPSessionManager(), rib, new FlowStateGroupEncoder());
		impl.setIPCProcess(ipc);
		impl.setAlgorithm(new DijkstraAlgorithm(), new Vertex(1));
		
		impl.flowAllocated(1, 1, 2, 1);
		impl.enrollmentToNeighbor(new ApplicationProcessNamingInformation(), 3, true, 2);
		
		Assert.assertEquals(rib.recoveredObject.getFlowStateObjectArray().get(0).getAddress(), 1);
		Assert.assertEquals(rib.recoveredObject.getFlowStateObjectArray().get(0).getPortid(), 1);
		Assert.assertEquals(rib.recoveredObject.getFlowStateObjectArray().get(0).getNeighborAddress(), 2);
		Assert.assertEquals(rib.recoveredObject.getFlowStateObjectArray().get(0).getNeighborPortid(), 1);
	}
}
