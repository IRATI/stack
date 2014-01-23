package unitTest.PDUForwardingTable.internalobjects;


import static org.junit.Assert.*;

import org.junit.Test;

import rina.ipcprocess.impl.PDUForwardingTable.internalobjects.FlowStateInternalObject;
import rina.ipcprocess.impl.PDUForwardingTable.internalobjects.FlowStateInternalObjectGroup;

public class FlowStateInternalObjectGroupTest {

	@Test
	public void add_AddObject_Added()
	{
		long address = 1;
		int portId = 1;
		long neighborAddress = 2;
		int neigborPortID = 1;
		boolean state = true;
		int sequenceNumber = 1;
		int age = 1;
		FlowStateInternalObject o1 = new FlowStateInternalObject(address, portId, neighborAddress, neigborPortID, state, sequenceNumber, age);
		FlowStateInternalObjectGroup g1 = new FlowStateInternalObjectGroup();
		
		g1.add(o1);
		
		assertEquals(g1.getFlowStateObjectArray().get(0), o1);
	}
	
	@Test
	public void FlowStateInternalObjectGroup_ParameterConstructor_Constructed()
	{
		long address = 1;
		int portId = 1;
		long neighborAddress = 2;
		int neigborPortID = 1;
		boolean state = true;
		int sequenceNumber = 1;
		int age = 1;
		FlowStateInternalObject o1 = new FlowStateInternalObject(address, portId, neighborAddress, neigborPortID, state, sequenceNumber, age);
		FlowStateInternalObjectGroup g1 = new FlowStateInternalObjectGroup();
		
		g1.add(o1);
		FlowStateInternalObjectGroup g2 = new FlowStateInternalObjectGroup(g1.getFlowStateObjectArray());
		
		assertEquals(g1.getFlowStateObjectArray(), g2.getFlowStateObjectArray());
	}
	
	@Test
	public void getByPortId_NoObjects_Null()
	{
		FlowStateInternalObjectGroup g1 = new FlowStateInternalObjectGroup();
		
		assertEquals(g1.getByPortId(1), null);
	}
	
	@Test
	public void getByPortId_RetrieveObjectAdded_EqualObjectAdded()
	{
		long address = 1;
		int portId = 1;
		long neighborAddress = 2;
		int neigborPortID = 1;
		boolean state = true;
		int sequenceNumber = 1;
		int age = 1;
		FlowStateInternalObject o1 = new FlowStateInternalObject(address, portId, neighborAddress, neigborPortID, state, sequenceNumber, age);
		FlowStateInternalObjectGroup g1 = new FlowStateInternalObjectGroup();
		
		g1.add(o1);
		
		assertEquals(g1.getByPortId(portId), o1);
	}
	
	@Test
	public void getByPortId_ObjectNotFound_Null()
	{
		long address = 1;
		int portId = 1;
		long neighborAddress = 2;
		int neigborPortID = 1;
		boolean state = true;
		int sequenceNumber = 1;
		int age = 1;
		FlowStateInternalObject o1 = new FlowStateInternalObject(address, portId, neighborAddress, neigborPortID, state, sequenceNumber, age);
		FlowStateInternalObjectGroup g1 = new FlowStateInternalObjectGroup();
		
		g1.add(o1);
		
		assertEquals(g1.getByPortId(2), null);
	}
	
	@Test
	public void getModifiedFSO_ObjectAdded_Object()
	{
		long address = 1;
		int portId = 1;
		long neighborAddress = 2;
		int neigborPortID = 1;
		boolean state = true;
		int sequenceNumber = 1;
		int age = 1;
		FlowStateInternalObject o1 = new FlowStateInternalObject(address, portId, neighborAddress, neigborPortID, state, sequenceNumber, age);
		FlowStateInternalObjectGroup g1 = new FlowStateInternalObjectGroup();
		
		g1.add(o1);
		
		assertEquals(g1.getModifiedFSO().get(0), o1);
	}
	
	@Test
	public void getModifiedFSO_ObjectsAdded_Objects()
	{
		long address = 1;
		int portId = 1;
		long neighborAddress = 2;
		int neigborPortID = 1;
		boolean state = true;
		int sequenceNumber = 1;
		int age = 1;
		FlowStateInternalObject o1 = new FlowStateInternalObject(address, portId, neighborAddress, neigborPortID, state, sequenceNumber, age);
		FlowStateInternalObject o2 = new FlowStateInternalObject(address, portId, neighborAddress, neigborPortID, state, sequenceNumber, age);
		FlowStateInternalObjectGroup g1 = new FlowStateInternalObjectGroup();
		
		g1.add(o1);
		g1.add(o2);
		
		assertEquals(g1.getModifiedFSO().get(0), o1);
		assertEquals(g1.getModifiedFSO().get(1), o2);
	}
	
	@Test
	public void getModifiedFSO_2Objects1notModified_ObjectModified()
	{
		long address = 1;
		int portId = 1;
		long neighborAddress = 2;
		int neigborPortID = 1;
		boolean state = true;
		int sequenceNumber = 1;
		int age = 1;
		FlowStateInternalObject o1 = new FlowStateInternalObject(address, portId, neighborAddress, neigborPortID, state, sequenceNumber, age);
		FlowStateInternalObject o2 = new FlowStateInternalObject(address, portId, neighborAddress, neigborPortID, state, sequenceNumber, age);
		FlowStateInternalObjectGroup g1 = new FlowStateInternalObjectGroup();
		
		o1.setModified(false);
		g1.add(o1);
		g1.add(o2);
		
		assertEquals(g1.getModifiedFSO().get(0), o2);
	}
	
	@Test
	public void getModifiedFSO_NoObjects_null()
	{
		FlowStateInternalObjectGroup g1 = new FlowStateInternalObjectGroup();
		
		assertEquals(g1.getModifiedFSO(), null);
	}
	
	@Test
	public void getModifiedFSO_ObjectnotModified_null()
	{
		long address = 1;
		int portId = 1;
		long neighborAddress = 2;
		int neigborPortID = 1;
		boolean state = true;
		int sequenceNumber = 1;
		int age = 1;
		FlowStateInternalObject o1 = new FlowStateInternalObject(address, portId, neighborAddress, neigborPortID, state, sequenceNumber, age);
		FlowStateInternalObjectGroup g1 = new FlowStateInternalObjectGroup();
		
		o1.setModified(false);
		g1.add(o1);
		
		assertEquals(g1.getModifiedFSO(), null);
	}
	
	@Test
	public void incrementAge_2Objects_Incremented()
	{
		long address = 1;
		int portId = 1;
		long neighborAddress = 2;
		int neigborPortID = 1;
		boolean state = true;
		int sequenceNumber = 1;
		int age = 1;
		FlowStateInternalObject o1 = new FlowStateInternalObject(address, portId, neighborAddress, neigborPortID, state, sequenceNumber, age);
		age = 30;
		FlowStateInternalObject o2 = new FlowStateInternalObject(address, portId, neighborAddress, neigborPortID, state, sequenceNumber, age);
		FlowStateInternalObjectGroup g1 = new FlowStateInternalObjectGroup();
		
		g1.add(o1);
		g1.add(o2);
		g1.incrementAge(200);
		
		assertEquals(g1.getFlowStateObjectArray().get(0).getAge(), 2);
		assertEquals(g1.getFlowStateObjectArray().get(1).getAge(), 31);
	}
	
	@Test
	public void incrementAge_2ObjectsIncremented_false()
	{
		long address = 1;
		int portId = 1;
		long neighborAddress = 2;
		int neigborPortID = 1;
		boolean state = true;
		int sequenceNumber = 1;
		int age = 1;
		FlowStateInternalObject o1 = new FlowStateInternalObject(address, portId, neighborAddress, neigborPortID, state, sequenceNumber, age);
		age = 30;
		FlowStateInternalObject o2 = new FlowStateInternalObject(address, portId, neighborAddress, neigborPortID, state, sequenceNumber, age);
		FlowStateInternalObjectGroup g1 = new FlowStateInternalObjectGroup();
		
		g1.add(o1);
		g1.add(o2);
		
		assertEquals(g1.incrementAge(200), false);
	}
	
	@Test
	public void incrementAge_EraseObjectMaximumAge_EmptyList()
	{
		long address = 1;
		int portId = 1;
		long neighborAddress = 2;
		int neigborPortID = 1;
		boolean state = true;
		int sequenceNumber = 1;
		int age = 30;
		FlowStateInternalObject o1 = new FlowStateInternalObject(address, portId, neighborAddress, neigborPortID, state, sequenceNumber, age);
		FlowStateInternalObjectGroup g1 = new FlowStateInternalObjectGroup();
		
		g1.add(o1);
		g1.incrementAge(31);
		
		assertEquals(g1.getFlowStateObjectArray().size(), 0);
	}
	
	public void incrementAge_EraseObjectMaximumAge_true()
	{
		long address = 1;
		int portId = 1;
		long neighborAddress = 2;
		int neigborPortID = 1;
		boolean state = true;
		int sequenceNumber = 1;
		int age = 30;
		FlowStateInternalObject o1 = new FlowStateInternalObject(address, portId, neighborAddress, neigborPortID, state, sequenceNumber, age);
		FlowStateInternalObjectGroup g1 = new FlowStateInternalObjectGroup();
		
		g1.add(o1);
		
		assertEquals(g1.incrementAge(31), true);
	}
	
	public void incrementAge_EmptyList_false()
	{
		FlowStateInternalObjectGroup g1 = new FlowStateInternalObjectGroup();
		
		assertEquals(g1.incrementAge(31), false);
	}
}
