package unitTest.PDUForwardingTable.timertasks;

import static org.junit.Assert.*;
import junit.framework.Assert;

import rina.encoding.impl.googleprotobuf.flowstate.FlowStateEncoder;
import rina.ipcprocess.impl.PDUForwardingTable.PDUFTImpl;
import rina.ipcprocess.impl.PDUForwardingTable.PDUFTInt;
import rina.ipcprocess.impl.PDUForwardingTable.routingalgorithms.dijkstra.DijkstraAlgorithm;
import rina.ipcprocess.impl.PDUForwardingTable.routingalgorithms.dijkstra.Vertex;
import rina.ipcprocess.impl.PDUForwardingTable.timertasks.ComputePDUFT;
import unitTest.PDUForwardingTable.fakeobjects.FakeCDAPSessionManager;
import unitTest.PDUForwardingTable.fakeobjects.FakeIPCProcess;
import unitTest.PDUForwardingTable.fakeobjects.FakeRIBDaemon;

import org.junit.Before;
import org.junit.Test;

public class computePDUFTTest {
	
	protected ComputePDUFT compute = null;
	protected PDUFTInt impl = null;

	@Before
	public void setup()
	{
		impl = new PDUFTImpl(2147483647);
		compute = new ComputePDUFT(impl);
	}
	//@Test
	public void run_() {
		impl.setIPCProcess(new FakeIPCProcess(new FakeCDAPSessionManager(), new FakeRIBDaemon(), new FlowStateEncoder()));
		impl.setAlgorithm(new DijkstraAlgorithm(), new Vertex(1));
		
		impl.flowAllocated(1, 1, 2, 1);
	}

}
