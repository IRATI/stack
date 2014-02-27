package unitTest.PDUForwardingTable.routingalgorithms.dijkstra;


import static org.junit.Assert.*;

import java.util.ArrayList;

import org.junit.Test;

import rina.PDUForwardingTable.api.FlowStateObject;
import rina.ipcprocess.impl.PDUForwardingTable.routingalgorithms.dijkstra.Graph;
import rina.ipcprocess.impl.PDUForwardingTable.routingalgorithms.dijkstra.Vertex;

public class GraphTest {

	@Test
	public void Graph_EmptyGraph_Empty()
	{
		
		ArrayList<FlowStateObject> a1 = new ArrayList<FlowStateObject>();
		Graph g = new Graph(a1);
		
		assertEquals(g.getVertices().size(), 0);
		assertEquals(g.getEdges().size(), 0);
	}

	@Test
	public void Graph_Contruct2Nodes_True()
	{
		long address = 1;
		int portId = 1;
		long neighborAddress = 2;
		int neigborPortID = 1;
		boolean state = true;
		int sequenceNumber = 1;
		int age = 1;
		
		FlowStateObject o1 = new FlowStateObject(address, portId, neighborAddress, neigborPortID, state, sequenceNumber, age);
		address = 2;
		neighborAddress = 1;
		FlowStateObject o2 = new FlowStateObject(address, portId, neighborAddress, neigborPortID, state, sequenceNumber, age);
		ArrayList<FlowStateObject> a = new ArrayList<FlowStateObject>();
		a.add(o1);
		a.add(o2);
		Graph g = new Graph(a);
		
		Vertex o1Vertex = new Vertex(o1.getAddress());
		Vertex o2Vertex = new Vertex(o2.getAddress());
		boolean o1Check = g.getVertices().get(0).equals(o1Vertex) || g.getVertices().get(1).equals(o1Vertex);
		boolean o2Check = g.getVertices().get(0).equals(o2Vertex) || g.getVertices().get(1).equals(o2Vertex);
		assertTrue(o1Check);
		assertTrue(o2Check);
	}
	
	@Test
	public void Graph_StateFalseisEmpty_True()
	{
		long address = 1;
		int portId = 1;
		long neighborAddress = 2;
		int neigborPortID = 1;
		boolean state = true;
		int sequenceNumber = 1;
		int age = 1;
		
		FlowStateObject o1 = new FlowStateObject(address, portId, neighborAddress, neigborPortID, state, sequenceNumber, age);
		address = 2;
		neighborAddress = 1;
		FlowStateObject o2 = new FlowStateObject(address, portId, neighborAddress, neigborPortID, false, sequenceNumber, age);
		ArrayList<FlowStateObject> a = new ArrayList<FlowStateObject>();
		a.add(o1);
		a.add(o2);
		Graph g = new Graph(a);
		
		assertTrue(g.getEdges().isEmpty());
	}
	
	@Test
	public void Graph_NotConnected_True()
	{	
		FlowStateObject o1 = new FlowStateObject(1, 1, 2, 1, true, 1, 1);
		FlowStateObject o2 = new FlowStateObject(2, 2, 3, 2, true, 1, 1);
		FlowStateObject o3 = new FlowStateObject(3, 1, 1, 2, true, 1, 1);
		ArrayList<FlowStateObject> a = new ArrayList<FlowStateObject>();
		a.add(o1);
		a.add(o2);
		a.add(o3);
		Graph g = new Graph(a);
		
		assertEquals(g.getVertices().size(), 3);
		assertTrue(g.getEdges().isEmpty());
	}
	
	@Test
	public void Graph_ContructNoBiderectionalFlow_False()
	{
		long address = 1;
		int portId = 1;
		long neighborAddress = 2;
		int neigborPortID = 1;
		boolean state = true;
		int sequenceNumber = 1;
		int age = 1;
		
		FlowStateObject o1 = new FlowStateObject(address, portId, neighborAddress, neigborPortID, state, sequenceNumber, age);
		address = 2;
		ArrayList<FlowStateObject> a = new ArrayList<FlowStateObject>();
		a.add(o1);
		Graph g = new Graph(a);
		
		assertEquals(g.getVertices().size(), 2);
		assertTrue(g.getEdges().isEmpty());
	}
	
	@Test
	public void Graph_ContructTriangleGraph_True()
	{
		boolean o1Check = false;
		boolean o2Check = false;
		boolean o3Check = false;
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
		//	Second Flow node 1 and node 3	
		address = 1;
		portId = 2;
		neighborAddress = 3;
		neighborPortID = 2;
		FlowStateObject o3 = new FlowStateObject(address, portId, neighborAddress, neighborPortID, state, sequenceNumber, age);
		address = 3;
		neighborAddress = 1;
		FlowStateObject o4 = new FlowStateObject(address, portId, neighborAddress, neighborPortID, state, sequenceNumber, age);
		a.add(o3);
		a.add(o4);
		//	Third Flow	node 2 and node 3	
		address = 2;
		portId = 3;
		neighborAddress = 3;
		neighborPortID = 3;
		FlowStateObject o5 = new FlowStateObject(address, portId, neighborAddress, neighborPortID, state, sequenceNumber, age);
		address = 3;
		neighborAddress = 2;
		FlowStateObject o6 = new FlowStateObject(address, portId, neighborAddress, neighborPortID, state, sequenceNumber, age);
		a.add(o5);
		a.add(o6);
		//	Graph Creation
		Graph g = new Graph(a);
		
		Vertex flow1o1Vertex = new Vertex(o1.getAddress());
		Vertex flow1o2Vertex = new Vertex(o2.getAddress());
		Vertex flow2o3Vertex = new Vertex(o3.getAddress());
		
		for ( Vertex v : g.getVertices())
		{
			o1Check = o1Check || v.equals(flow1o1Vertex);
			o2Check = o2Check || v.equals(flow1o2Vertex);
			o3Check = o3Check || v.equals(flow2o3Vertex);
		}
		assertEquals(g.getVertices().size(), 3);
		assertTrue(o1Check);
		assertTrue(o2Check);
		assertTrue(o3Check);
		assertEquals(g.getEdges().size(), 3);
	}
}
