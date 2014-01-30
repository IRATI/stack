package rina.ipcprocess.impl.PDUForwardingTable.internalobjects;

import java.util.ArrayList;
import java.util.List;

import rina.PDUForwardingTable.api.*;

public class ObjectStateMapper {
	
	public FlowStateObject FSOMap (FlowStateInternalObject intObj)
	{
		return new FlowStateObject(intObj.getAddress(), intObj.getPortid(), intObj.getNeighborAddress(), intObj.getNeighborPortid(), intObj.isState(), intObj.getSequenceNumber(), intObj.getAge());
	}
	
	public FlowStateObjectGroup FSOGMap (FlowStateInternalObjectGroup intObj)
	{
		ArrayList<FlowStateInternalObject> srcObj = intObj.getFlowStateObjectArray();
		ArrayList<FlowStateObject> destObj = new ArrayList<FlowStateObject>();
		
		for (int i = 0; i < srcObj.size() ; i++)
		{
			destObj.add(FSOMap(srcObj.get(i)));
		}
		
		return new FlowStateObjectGroup(destObj);
	}
	
	public FlowStateInternalObject FSOMap (FlowStateObject intObj)
	{
		return new FlowStateInternalObject(intObj.getAddress(), intObj.getPortid(), intObj.getNeighborAddress(), intObj.getNeighborPortid(), intObj.isState(), intObj.getSequenceNumber(), intObj.getAge());
	}
	
	public FlowStateInternalObjectGroup FSOGMap (FlowStateObjectGroup intObj)
	{
		List<FlowStateObject> srcObj = intObj.getFlowStateObjectArray();
		ArrayList<FlowStateInternalObject> destObj = new ArrayList<FlowStateInternalObject>();
		
		for (int i = 0; i < srcObj.size() ; i++)
		{
			destObj.add(FSOMap(srcObj.get(i)));
		}
		
		return new FlowStateInternalObjectGroup(destObj);
	}

}
