package unit.test.PDUForwardingTable.internalobjects;

import static org.junit.Assert.*;
import org.junit.Test;

import rina.ipcprocess.impl.PDUForwardingTable.internalobjects.FlowStateInternalObject;

public class FlowStateInternalObjectTest {

	@Test
	public void FlowStateInternalObject_address_Equal()
	{
		long address = 1;
		int portId = 1;
		long neighborAddress = 2;
		int neigborPortID = 1;
		boolean state = true;
		int sequenceNumber = 1;
		int age = 1;
		
		FlowStateInternalObject o1 = new FlowStateInternalObject(address, portId, neighborAddress, neigborPortID, state, sequenceNumber, age);
		
		assertEquals(o1.getAddress(), address);
	}
	
	@Test
	public void FlowStateInternalObject_portId_Equal()
	{
		long address = 1;
		int portId = 1;
		long neighborAddress = 2;
		int neigborPortID = 1;
		boolean state = true;
		int sequenceNumber = 1;
		int age = 1;
		
		FlowStateInternalObject o1 = new FlowStateInternalObject(address, portId, neighborAddress, neigborPortID, state, sequenceNumber, age);
		
		assertEquals(o1.getPortid(), portId);
	}
	
	@Test
	public void FlowStateInternalObject_neighborAddress_Equal()
	{
		long address = 1;
		int portId = 1;
		long neighborAddress = 2;
		int neigborPortID = 1;
		boolean state = true;
		int sequenceNumber = 1;
		int age = 1;
		
		FlowStateInternalObject o1 = new FlowStateInternalObject(address, portId, neighborAddress, neigborPortID, state, sequenceNumber, age);
		
		assertEquals(o1.getNeighborAddress(), neighborAddress);
	}
	
	@Test
	public void FlowStateInternalObject_neigborPortID_Equal()
	{
		long address = 1;
		int portId = 1;
		long neighborAddress = 2;
		int neigborPortID = 1;
		boolean state = true;
		int sequenceNumber = 1;
		int age = 1;
		
		FlowStateInternalObject o1 = new FlowStateInternalObject(address, portId, neighborAddress, neigborPortID, state, sequenceNumber, age);
		
		assertEquals(o1.getNeighborPortid(), neigborPortID);
	}
	
	@Test
	public void FlowStateInternalObject_state_Equal()
	{
		long address = 1;
		int portId = 1;
		long neighborAddress = 2;
		int neigborPortID = 1;
		boolean state = true;
		int sequenceNumber = 1;
		int age = 1;
		
		FlowStateInternalObject o1 = new FlowStateInternalObject(address, portId, neighborAddress, neigborPortID, state, sequenceNumber, age);
		
		assertEquals(o1.isState(), state);
	}
	
	@Test
	public void FlowStateInternalObject_sequenceNumber_Equal()
	{
		long address = 1;
		int portId = 1;
		long neighborAddress = 2;
		int neigborPortID = 1;
		boolean state = true;
		int sequenceNumber = 1;
		int age = 1;
		
		FlowStateInternalObject o1 = new FlowStateInternalObject(address, portId, neighborAddress, neigborPortID, state, sequenceNumber, age);
		
		assertEquals(o1.getSequenceNumber(), sequenceNumber);
	}

	@Test
	public void FlowStateInternalObject_age_Equal()
	{
		long address = 1;
		int portId = 1;
		long neighborAddress = 2;
		int neigborPortID = 1;
		boolean state = true;
		int sequenceNumber = 1;
		int age = 1;
		
		FlowStateInternalObject o1 = new FlowStateInternalObject(address, portId, neighborAddress, neigborPortID, state, sequenceNumber, age);
		
		assertEquals(o1.getAge(), age);
	}
}
