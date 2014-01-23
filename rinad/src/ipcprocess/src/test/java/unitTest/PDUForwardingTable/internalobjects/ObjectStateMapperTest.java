package unitTest.PDUForwardingTable.internalobjects;


import static org.junit.Assert.*;

import java.util.ArrayList;

import org.junit.Test;

import rina.PDUForwardingTable.api.FlowStateObject;
import rina.PDUForwardingTable.api.FlowStateObjectGroup;
import rina.ipcprocess.impl.PDUForwardingTable.internalobjects.FlowStateInternalObject;
import rina.ipcprocess.impl.PDUForwardingTable.internalobjects.FlowStateInternalObjectGroup;
import rina.ipcprocess.impl.PDUForwardingTable.internalobjects.ObjectStateMapper;

public class ObjectStateMapperTest {

	@Test
	public void FSOMap_InternalToDummy_ParametersEqual()
	{
		long address = 1;
		int portId = 1;
		long neighborAddress = 2;
		int neigborPortID = 1;
		boolean state = true;
		int sequenceNumber = 1;
		int age = 1;
		
		ObjectStateMapper mapper = new ObjectStateMapper();
		FlowStateInternalObject o1 = new FlowStateInternalObject(address, portId, neighborAddress, neigborPortID, state, sequenceNumber, age);
		FlowStateObject o2 = mapper.FSOMap(o1);
		
		assertEquals(o1.getAddress(), o2.getAddress());
		assertEquals(o1.getPortid(), o2.getPortid());
		assertEquals(o1.getNeighborAddress(), o2.getNeighborAddress());
		assertEquals(o1.getPortid(), o2.getPortid());
		assertEquals(o1.isState(), o2.isState());
		assertEquals(o1.getSequenceNumber(), o2.getSequenceNumber());
		assertEquals(o1.getAge(), o2.getAge());
	}
	
	@Test
	public void FSOMap_DummyToInternal_ParametersEqual()
	{
		long address = 1;
		int portId = 1;
		long neighborAddress = 2;
		int neigborPortID = 1;
		boolean state = true;
		int sequenceNumber = 1;
		int age = 1;
		
		ObjectStateMapper mapper = new ObjectStateMapper();
		FlowStateObject o1 = new FlowStateObject(address, portId, neighborAddress, neigborPortID, state, sequenceNumber, age);
		FlowStateInternalObject o2 = mapper.FSOMap(o1);
		
		assertEquals(o1.getAddress(), o2.getAddress());
		assertEquals(o1.getPortid(), o2.getPortid());
		assertEquals(o1.getNeighborAddress(), o2.getNeighborAddress());
		assertEquals(o1.getPortid(), o2.getPortid());
		assertEquals(o1.isState(), o2.isState());
		assertEquals(o1.getSequenceNumber(), o2.getSequenceNumber());
		assertEquals(o1.getAge(), o2.getAge());
	}

	@Test
	public void FSOGMap_DummyToInternal_ParametersEqual()
	{
		long address = 1;
		int portId = 1;
		long neighborAddress = 2;
		int neigborPortID = 1;
		boolean state = true;
		int sequenceNumber = 1;
		int age = 1;
		
		ObjectStateMapper mapper = new ObjectStateMapper();
		FlowStateObject o1 = new FlowStateObject(address, portId, neighborAddress, neigborPortID, state, sequenceNumber, age);
		ArrayList<FlowStateObject> al = new ArrayList<FlowStateObject>();
		al.add(o1);
		FlowStateObjectGroup g1 = new FlowStateObjectGroup();
		g1.setFlowStateObjectArray(al);
		FlowStateInternalObjectGroup g2 = mapper.FSOGMap(g1);
		
		assertEquals(o1.getAddress(), g2.getFlowStateObjectArray().get(0).getAddress());
		assertEquals(o1.getPortid(), g2.getFlowStateObjectArray().get(0).getPortid());
		assertEquals(o1.getNeighborAddress(), g2.getFlowStateObjectArray().get(0).getNeighborAddress());
		assertEquals(o1.getPortid(), g2.getFlowStateObjectArray().get(0).getPortid());
		assertEquals(o1.isState(), g2.getFlowStateObjectArray().get(0).isState());
		assertEquals(o1.getSequenceNumber(), g2.getFlowStateObjectArray().get(0).getSequenceNumber());
		assertEquals(o1.getAge(), g2.getFlowStateObjectArray().get(0).getAge());
	}
	
	@Test
	public void FSOGMap_InternalToDummy_ParametersEqual()
	{
		long address = 1;
		int portId = 1;
		long neighborAddress = 2;
		int neigborPortID = 1;
		boolean state = true;
		int sequenceNumber = 1;
		int age = 1;
		
		ObjectStateMapper mapper = new ObjectStateMapper();
		FlowStateInternalObject o1 = new FlowStateInternalObject(address, portId, neighborAddress, neigborPortID, state, sequenceNumber, age);
		FlowStateInternalObjectGroup g1 = new FlowStateInternalObjectGroup();
		g1.add(o1);
		FlowStateObjectGroup g2 = mapper.FSOGMap(g1);
		
		assertEquals(o1.getAddress(), g2.getFlowStateObjectArray().get(0).getAddress());
		assertEquals(o1.getPortid(), g2.getFlowStateObjectArray().get(0).getPortid());
		assertEquals(o1.getNeighborAddress(), g2.getFlowStateObjectArray().get(0).getNeighborAddress());
		assertEquals(o1.getPortid(), g2.getFlowStateObjectArray().get(0).getPortid());
		assertEquals(o1.isState(), g2.getFlowStateObjectArray().get(0).isState());
		assertEquals(o1.getSequenceNumber(), g2.getFlowStateObjectArray().get(0).getSequenceNumber());
		assertEquals(o1.getAge(), g2.getFlowStateObjectArray().get(0).getAge());
	}
}
