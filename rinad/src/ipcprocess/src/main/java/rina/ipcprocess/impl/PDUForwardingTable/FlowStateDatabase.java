package rina.ipcprocess.impl.PDUForwardingTable;

import java.util.ArrayList;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import eu.irati.librina.FlowInformation;

import rina.PDUForwardingTable.api.FlowStateObjectGroup;
import rina.encoding.api.Encoder;
import rina.ipcprocess.impl.PDUForwardingTable.internalobjects.FlowStateInternalObject;
import rina.ipcprocess.impl.PDUForwardingTable.internalobjects.FlowStateInternalObjectGroup;
import rina.ipcprocess.impl.PDUForwardingTable.internalobjects.ObjectStateMapper;
import rina.ipcprocess.impl.PDUForwardingTable.ribobjects.FlowStateRIBObjectGroup;
import rina.ribdaemon.api.RIBDaemonException;

public class FlowStateDatabase {
	private static final Log log = LogFactory.getLog(FlowStateDatabase.class);
	protected final int NO_AVOID_PORT = -1;

	/**
	 * The FlowStateObjectGroup
	 */
	protected FlowStateInternalObjectGroup flowStateInternalObjectGroup = null;
	
	/**
	 * Bit to know if the FSDB has been modified
	 */
	protected boolean isModified = false;
	
	/**
	 * Address of this ipcprocess
	 */
	long address;
	
	/*		Accessors		*/
	public boolean isModified()
	{
		return this.isModified;
	}
	public void setModified(boolean isModified)
	{
		this.isModified = isModified;
	}
	public FlowStateInternalObjectGroup getFlowStateInternalObjectGroup() {
		return flowStateInternalObjectGroup;
	}
	public void setAddress(long address)
	{
		this.address = address;
	}
	/*		Constructors		*/
	public FlowStateDatabase()
	{
		this.flowStateInternalObjectGroup = new FlowStateInternalObjectGroup();
		this.isModified = false;
	}
	
	/*		Methods		*/
	public FlowStateObjectGroup getFlowStateObjectGroup() {
		ObjectStateMapper mapper =  new ObjectStateMapper();
		return mapper.FSOGMap(flowStateInternalObjectGroup);
	}
	
	public boolean isEmpty()
	{
		return flowStateInternalObjectGroup.getFlowStateObjectArray().size() == 0;
	}
	
	public byte[] encode (Encoder encoder) throws Exception
	{
		return encoder.encode(getFlowStateObjectGroup());
	}
	
	public void setAvoidPort(int avoidPort)
	{
		for (FlowStateInternalObject fsio: flowStateInternalObjectGroup.getFlowStateObjectArray())
		{
			fsio.setAvoidPort(avoidPort);
		}
	}
	
	public void addObjectToGroup(long address, int portId, long neighborAddress, int neighborPortId, FlowStateRIBObjectGroup fsRIBGroup )
	{
		
		try {
			this.flowStateInternalObjectGroup.add(new FlowStateInternalObject(address, portId, neighborAddress, neighborPortId, true, 1, 0)
				, fsRIBGroup);
			this.isModified = true;
		} catch (RIBDaemonException e) {
			log.debug("Error Code: " + e.getErrorCode());
			e.printStackTrace();
		}

	}
	
	public boolean deprecateObject(int portId, int maximumAge)
	{
		FlowStateInternalObject object = flowStateInternalObjectGroup.getByPortId(portId);
		if (object == null)
		{
			return false;
		}
		
		object.setState(false);
		object.setAge(maximumAge);
		object.setSequenceNumber(object.getSequenceNumber()+1);
		object.setAvoidPort(NO_AVOID_PORT);
		object.setModified(true);
		this.isModified = true;
		return true;
	}
	
	public ArrayList<FlowStateInternalObjectGroup> prepareForPropagation(FlowInformation[] flows)
	{
		ArrayList<FlowStateInternalObjectGroup> groupsToSend = new ArrayList<FlowStateInternalObjectGroup>();
		if(flowStateInternalObjectGroup.getModifiedFSO().size() > 0)
		{
			for(int i = 0; i < flows.length; i++)
			{
				groupsToSend.add(new FlowStateInternalObjectGroup());
			}
		
			for(int i = 0; i < flowStateInternalObjectGroup.getModifiedFSO().size(); i++)
			{
				FlowStateInternalObject object = flowStateInternalObjectGroup.getModifiedFSO().get(i);
				log.debug("Check modified object: " + object.getID() + " to be sent with age: " + object.getAge() + " and Status: " + object.isState());
	
				for(int j = 0; j < flows.length; j++)
				{
					int portId = flows[j].getPortId();
					if (object.getAvoidPort() != portId)
					{
						log.debug("Sent to port: " + portId);
						groupsToSend.get(j).addToSend(object);
					}
				}
				object.setModified(false);
				object.setAvoidPort(NO_AVOID_PORT);
			}
		}
		return groupsToSend;
	}
	

	public void incrementAge(int maximumAge, FlowStateRIBObjectGroup fsRIBGroup) throws RIBDaemonException
	{
		this.flowStateInternalObjectGroup.incrementAge(maximumAge, fsRIBGroup, this);
	}
	
	public void updateObjects(FlowStateObjectGroup newObjectGroup, int avoidPort, FlowStateRIBObjectGroup fsRIBGroup)
	{
		log.debug("Update Objects from DB launched");
		
		ObjectStateMapper mapper = new ObjectStateMapper();
		ArrayList<FlowStateInternalObject> newObjectInternalGroup= mapper.FSOGMap(newObjectGroup).getFlowStateObjectArray();
		ArrayList<FlowStateInternalObject> objects = this.flowStateInternalObjectGroup.getFlowStateObjectArray();
		boolean continueLoop = true;
		int i = 0;
		
		for(FlowStateInternalObject newObj : newObjectInternalGroup)
		{
			while (continueLoop && i < objects.size())
			{
				FlowStateInternalObject oldObj = objects.get(i);
				
				if (newObj.getAddress() == oldObj.getAddress() /*&& objM.getPortid() == obj.getPortid()*/
						&& newObj.getNeighborAddress() == oldObj.getNeighborAddress() 
						/*&& objM.getNeighborPortid() == obj.getNeighborPortid()*/)
				{
					log.debug("Found the object in the DB. Obj: " + newObj.getID());
					continueLoop = false;
					if (newObj.getAddress() == this.address)
					{
						log.debug("Object is self generated, updating sequence number of " + oldObj.getID());
						oldObj.setSequenceNumber(newObj.getSequenceNumber() + 1);
						oldObj.setModified(true);
						this.isModified = true;
					}
					if (newObj.getSequenceNumber() > oldObj.getSequenceNumber())
					{
						log.debug("Update the object: " + oldObj.getID());
						oldObj.setAge(newObj.getAge());
						oldObj.setState(newObj.isState());
						oldObj.setSequenceNumber(newObj.getSequenceNumber());
						oldObj.setModified(true);
						oldObj.setAvoidPort(avoidPort);
						
						this.isModified = true;
					}
				}
				
				i++;
			}
			if (continueLoop)
			{
				if (newObj.getAddress() == this.address)
				{
					log.debug("Object is self origin, discart object " + newObj.getID());
				}
				else
				{
					log.debug("New object added");
					newObj.setAvoidPort(avoidPort);
					newObj.setModified(true);
					try {
						this.flowStateInternalObjectGroup.add(newObj, fsRIBGroup);
					} catch (RIBDaemonException e) {
						log.error("could not add the object to the rib");
						e.printStackTrace();
					}
					this.isModified = true;
				}
			}
			continueLoop = true;
			i = 0;
		}
	}

}
