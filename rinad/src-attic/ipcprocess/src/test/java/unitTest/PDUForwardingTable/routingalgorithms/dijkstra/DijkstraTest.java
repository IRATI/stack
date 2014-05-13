package unitTest.PDUForwardingTable.routingalgorithms.dijkstra;


import static org.junit.Assert.*;

import java.util.ArrayList;

import org.junit.Test;

import eu.irati.librina.PDUForwardingTableEntry;
import eu.irati.librina.PDUForwardingTableEntryList;

import rina.PDUForwardingTable.api.FlowStateObject;
import rina.PDUForwardingTable.api.RoutingAlgorithmInt;
import rina.ipcprocess.impl.PDUForwardingTable.routingalgorithms.dijkstra.DijkstraAlgorithm;
import rina.ipcprocess.impl.PDUForwardingTable.routingalgorithms.dijkstra.Vertex;

public class DijkstraTest {

	static {
		System.loadLibrary("rina_java");
	}
	
	@Test
	public void getPDUTForwardingTable_NoFSO_size0()
	{		
		ArrayList<FlowStateObject> a = new ArrayList<FlowStateObject>();
		
		//	Dijkstra Algorithm	
		Vertex source = new Vertex(1); 
		RoutingAlgorithmInt algorithm = new DijkstraAlgorithm();
		PDUForwardingTableEntryList PDUFT = algorithm.getPDUTForwardingTable(a, source);

		assertEquals(PDUFT.size(), 0);
	}
	
	@Test
	public void getPDUTForwardingTable_LinearGraphNumberOfEntries_2()
	{
		long address = 1;
		int portId = 1;
		long neighborAddress = 2;
		int neighborPortID = 1;
		boolean state = true;
		int sequenceNumber = 1;
		int age = 1;
		
		ArrayList<FlowStateObject> a = new ArrayList<FlowStateObject>();
		//	First Flow	node 1 and node 2
		FlowStateObject o1 = new FlowStateObject(address, portId, neighborAddress, neighborPortID, state, sequenceNumber, age);
		address = 2;
		neighborAddress = 1;
		FlowStateObject o2 = new FlowStateObject(address, portId, neighborAddress, neighborPortID, state, sequenceNumber, age);
		a.add(o1);
		a.add(o2);
		//	Second Flow	node 2 and node 3
		address = 2;
		portId = 3;
		neighborAddress = 3;
		neighborPortID = 3;
		FlowStateObject o3 = new FlowStateObject(address, portId, neighborAddress, neighborPortID, state, sequenceNumber, age);
		address = 3;
		neighborAddress = 2;
		FlowStateObject o4 = new FlowStateObject(address, portId, neighborAddress, neighborPortID, state, sequenceNumber, age);
		a.add(o3);
		a.add(o4);
		//	Dijkstra Algorithm
		Vertex source = new Vertex(o1.getAddress()); 
		RoutingAlgorithmInt algorithm = new DijkstraAlgorithm();
		PDUForwardingTableEntryList PDUFT = algorithm.getPDUTForwardingTable(a, source);

		assertEquals(PDUFT.size(), 2);
	}

	@Test
	public void getPDUTForwardingTable_StateFalseNoEntries_True()
	{
		long address = 1;
		int portId = 1;
		long neighborAddress = 2;
		int neighborPortID = 1;
		
		ArrayList<FlowStateObject> a = new ArrayList<FlowStateObject>();
		
		/*	First Flow	node 1 and node 2	*/
		FlowStateObject o1 = new FlowStateObject(address, portId, neighborAddress, neighborPortID, true, 1, 1);
		FlowStateObject o2 = new FlowStateObject(neighborAddress, neighborPortID, address, portId, false, 1, 1);
		a.add(o1);
		a.add(o2);
		
		/*	Dijkstra Algorithm	*/
		Vertex source = new Vertex(o1.getAddress()); 
		RoutingAlgorithmInt algorithm = new DijkstraAlgorithm();
		PDUForwardingTableEntryList PDUFT = algorithm.getPDUTForwardingTable(a, source);

		assertTrue(PDUFT.isEmpty());
	}

	@Test
	public void getPDUTForwardingTable_LinearGraphEntries_True()
	{
		long address = 1;
		int portId = 1;
		long neighborAddress = 2;
		int neighborPortID = 1;
		boolean state = true;
		int sequenceNumber = 1;
		int age = 1;
		
		ArrayList<FlowStateObject> a = new ArrayList<FlowStateObject>();
		//	First Flow	node 1 and node 2
		FlowStateObject o1 = new FlowStateObject(address, portId, neighborAddress, neighborPortID, state, sequenceNumber, age);
		address = 2;
		neighborAddress = 1;
		FlowStateObject o2 = new FlowStateObject(address, portId, neighborAddress, neighborPortID, state, sequenceNumber, age);
		a.add(o1);
		a.add(o2);
		//	Second Flow	node 2 and node 3
		address = 2;
		portId = 3;
		neighborAddress = 3;
		neighborPortID = 3;
		FlowStateObject o3 = new FlowStateObject(address, portId, neighborAddress, neighborPortID, state, sequenceNumber, age);
		address = 3;
		neighborAddress = 2;
		FlowStateObject o4 = new FlowStateObject(address, portId, neighborAddress, neighborPortID, state, sequenceNumber, age);
		a.add(o3);
		a.add(o4);
		//	Dijkstra Algorithm
		Vertex source = new Vertex(o1.getAddress()); 
		RoutingAlgorithmInt algorithm = new DijkstraAlgorithm();
		PDUForwardingTableEntryList PDUFT = algorithm.getPDUTForwardingTable(a, source);

		PDUForwardingTableEntry e1 = PDUFT.getFirst();
		PDUForwardingTableEntry e2 = PDUFT.getLast();
		

		if (e1.getAddress() == 2)
		{
			assertEquals(e1.getAddress(), 2);
			assertEquals(e1.getPortIds().getFirst(), 1);
			assertEquals(e2.getAddress(), 3);
			assertEquals(e2.getPortIds().getFirst(), 1);
		}
		else
		{
			assertEquals(e1.getAddress(), 3);
			assertEquals(e1.getPortIds().getFirst(), 1);
			assertEquals(e2.getAddress(), 2);
			assertEquals(e2.getPortIds().getFirst(), 1);
		}

	}
	
	@Test
	public void getPDUTForwardingTable_LinearGraphPorts_True()
	{
		long address = 1;
		int portId = 1;
		long neighborAddress = 2;
		int neighborPortID = 2;
		boolean state = true;
		int sequenceNumber = 1;
		int age = 1;
		
		ArrayList<FlowStateObject> a = new ArrayList<FlowStateObject>();
		//	Flow	node 1 and node 2
		FlowStateObject o1 = new FlowStateObject(address, portId, neighborAddress, neighborPortID, state, sequenceNumber, age);
		FlowStateObject o2 = new FlowStateObject(neighborAddress, neighborPortID, address, portId, state, sequenceNumber, age);
		a.add(o1);
		a.add(o2);
		//	Dijkstra Algorithm
		Vertex source = new Vertex(o1.getAddress()); 
		RoutingAlgorithmInt algorithm = new DijkstraAlgorithm();
		PDUForwardingTableEntryList PDUFT = algorithm.getPDUTForwardingTable(a, source);

		PDUForwardingTableEntry e1 = PDUFT.getFirst();
		
		assertEquals(e1.getAddress(), 2);
		assertEquals(e1.getPortIds().getFirst(), 1);
	}
	
	@Test
	public void getPDUTForwardingTable_MultiGraphEntries_True()
	{
		long address = 1;
		int portId = 1;
		long neighborAddress = 2;
		int neighborPortID = 1;
		boolean state = true;
		int sequenceNumber = 1;
		int age = 1;
		
		ArrayList<FlowStateObject> a = new ArrayList<FlowStateObject>();
		//	First Flow	node 1 and node 2
		FlowStateObject o1 = new FlowStateObject(address, portId, neighborAddress, neighborPortID, state, sequenceNumber, age);
		address = 2;
		neighborAddress = 1;
		FlowStateObject o2 = new FlowStateObject(address, portId, neighborAddress, neighborPortID, state, sequenceNumber, age);
		a.add(o1);
		a.add(o2);
		//	Second Flow	node 2 and node 3
		address = 2;
		portId = 3;
		neighborAddress = 3;
		neighborPortID = 3;
		FlowStateObject o3 = new FlowStateObject(address, portId, neighborAddress, neighborPortID, state, sequenceNumber, age);
		address = 3;
		neighborAddress = 2;
		FlowStateObject o4 = new FlowStateObject(address, portId, neighborAddress, neighborPortID, state, sequenceNumber, age);
		a.add(o3);
		a.add(o4);
		//	Third Flow	node 1 and node 2
		address = 1;
		portId = 2;
		neighborAddress = 2;
		neighborPortID = 2;
		FlowStateObject o5 = new FlowStateObject(address, portId, neighborAddress, neighborPortID, state, sequenceNumber, age);
		address = 2;
		neighborAddress = 1;
		FlowStateObject o6 = new FlowStateObject(address, portId, neighborAddress, neighborPortID, state, sequenceNumber, age);
		a.add(o5);
		a.add(o6);
		
		
		//	Dijkstra Algorithm
		Vertex source = new Vertex(o1.getAddress()); 
		RoutingAlgorithmInt algorithm = new DijkstraAlgorithm();
		PDUForwardingTableEntryList PDUFT = algorithm.getPDUTForwardingTable(a, source);

		PDUForwardingTableEntry e1 = PDUFT.getFirst();
		PDUForwardingTableEntry e2 = PDUFT.getLast();
		

		if (e1.getAddress() == 2)
		{
			assertEquals(e1.getAddress(), 2);
			boolean checkPort1 = e1.getPortIds().getFirst() == 1 || e1.getPortIds().getFirst() == 2;
			assertTrue(checkPort1);
			assertEquals(e2.getAddress(), 3);
			boolean checkPort2 = e2.getPortIds().getFirst() == 1 || e1.getPortIds().getFirst() == 2;
			assertTrue(checkPort2);
		}
		else
		{
			assertEquals(e1.getAddress(), 3);
			boolean checkPort1 = e1.getPortIds().getFirst() == 1 || e1.getPortIds().getFirst() == 2;
			assertTrue(checkPort1);
			assertEquals(e2.getAddress(), 2);
			boolean checkPort2 = e2.getPortIds().getFirst() == 1 || e1.getPortIds().getFirst() == 2;
			assertTrue(checkPort2);
		}
	}

}
