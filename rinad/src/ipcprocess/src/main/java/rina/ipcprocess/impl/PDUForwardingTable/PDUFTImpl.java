package rina.ipcprocess.impl.PDUForwardingTable;

import rina.PDUForwardingTable.api.FlowStateObject;
import rina.PDUForwardingTable.api.FlowStateObjectGroup;
import rina.cdap.api.CDAPSessionManager;
import rina.encoding.api.Encoder;
import rina.ipcprocess.impl.IPCProcess;
import rina.ipcprocess.impl.PDUForwardingTable.internalobjects.*;
import rina.ipcprocess.impl.PDUForwardingTable.routingalgorithms.*;
import rina.ribdaemon.api.RIBDaemon;

import eu.irati.librina.ApplicationProcessNamingInformation;
import eu.irati.librina.FlowInformation;

import java.util.ArrayList;
import java.util.List;
import java.util.TimerTask;

public class PDUFTImpl /*implements <TODO>*/{
	
	protected final int NO_AVOID_PORT = -1;
	
	protected AbstractVertex sourceVertex;
	protected int maximumAge = 2147483647; 

	protected RoutingAlgorithmInt routingAlgorithm;
	/**
	 * The IPC process
	 */
	protected IPCProcess ipcProcess = null;
	
	/**
	 * The RIB Daemon
	 */
	protected RIBDaemon ribDaemon = null;
	
	private Encoder encoder = null;
	
	private CDAPSessionManager cdapSessionManager = null;
	
	/**
	 * The Flow State Database
	 */
	protected FlowStateDatabase db = null;
	
	/**
	 * Constructor
	 */
	public PDUFTImpl (IPCProcess ipcProcess, RIBDaemon ribDaemon, RoutingAlgorithmInt routingAlgorithm, AbstractVertex sourceVertex)
	{
		// TODO: Fer els sets dels atributs mirar enrollment Mirar events
		this.ipcProcess = ipcProcess;
		this.ribDaemon = ribDaemon;
		this.routingAlgorithm = routingAlgorithm;
		this.db = new FlowStateDatabase();
		this.sourceVertex = sourceVertex;
	}
	
	public void setIPCProcess(IPCProcess ipcProcess){
		this.ipcProcess = ipcProcess;
		encoder = ipcProcess.getEncoder();
		cdapSessionManager = ipcProcess.getCDAPSessionManager();
		//registrationManager = ipcProcess.getRegistrationManager();
		//populateRIB(ipcProcess);
	}
	
	
	
	public boolean enrollmentToNeighbor(ApplicationProcessNamingInformation name, long address, boolean newMember, int portId, TimerTask timer)
	{
		this.db.getFlowStateObjectGroup();
		/* Send CDAP with all FSO */
		/* Create Timer */
		return true;
	}

	public boolean flowAllocated(long address, int portId, long neighborAddress, int neighborPortId)
	{
		FlowStateInternalObject object = new FlowStateInternalObject(address, portId, neighborAddress, neighborPortId, true, 1, 0);
		
		return this.db.addObjectToGroup(object);
	}
	
	public boolean flowdeAllocated(int portId)
	{
		FlowStateInternalObject object = this.db.getByPortId(portId);
		if (object == null)
		{
			return false;
		}
		
		object.setState(false);
		object.setAge(maximumAge);
		object.setSequenceNumber(object.getSequenceNumber()+1);
		object.setAvoidPort(NO_AVOID_PORT);
		object.setModified(true);
		
		this.db.setModified(true);
		return true;
	}
	
	public boolean propagateFSDB()
	{
		ArrayList<FlowStateInternalObject> modifiedFSOs;
		ArrayList<Integer> portsToSend;
		
		if (!this.db.isModified())
		{
			return true;
		}
		
		modifiedFSOs = this.db.getModifiedFSO();
	
		for(int i = 0; i < modifiedFSOs.size(); i++)
		{
			FlowStateInternalObject object = modifiedFSOs.get(i);
			FlowInformation[] nminusFlowInfo = ipcProcess.getResourceAllocator().getNMinus1FlowManager().getAllNMinus1FlowsInformation();
			/*l
			portsToSend
			
			for(int port : portsToSend)
			{
				if (object.getAvoidPort != port)
				{
					Send the Object
				}
			}
			object.setIsModified(false);
			object.setAvoidPort = NO_AVOID_PORT;
			*/
		}
		
		this.db.setModified(false);
		return true;
	}
	
	public void updateAge()
	{
		this.db.incrementAge(maximumAge);
	}
	
	public boolean writeMessage(ArrayList<FlowStateObject> objectsToModify, int srcPort)
	{
		ObjectStateMapper mapper = new ObjectStateMapper();
		
		FlowStateInternalObjectGroup intObjectsToModify = mapper.FSOGMap(new FlowStateObjectGroup(objectsToModify));
		
		this.db.updateObjects(intObjectsToModify, srcPort);
		return true;
	}
	
	public void ForwardingTableupdate ()
	{
		if (this.db.isModified())
		{
			ObjectStateMapper  mapper = new ObjectStateMapper();
			List<FlowStateObject> fsoList = mapper.FSOGMap(this.db.getFlowStateObjectGroup()).getFlowStateObjectArray();
			routingAlgorithm.getPDUTForwardingTable(fsoList, this.sourceVertex);
		}
	}
}
