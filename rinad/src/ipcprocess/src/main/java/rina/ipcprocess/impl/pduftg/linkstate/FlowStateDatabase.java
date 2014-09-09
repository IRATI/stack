package rina.ipcprocess.impl.pduftg.linkstate;

import java.util.ArrayList;
import java.util.List;
import java.util.Timer;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import eu.irati.librina.FlowInformation;

import rina.encoding.api.Encoder;
import rina.ipcprocess.impl.pduftg.linkstate.ribobjects.FlowStateRIBObjectGroup;
import rina.ipcprocess.impl.pduftg.linkstate.timertasks.KillFlowStateObject;
import rina.pduftg.api.linkstate.FlowStateObject;
import rina.pduftg.api.linkstate.FlowStateObjectGroup;
import rina.ribdaemon.api.ObjectInstanceGenerator;
import rina.ribdaemon.api.RIBDaemonException;

public class FlowStateDatabase {
	private static final Log log = LogFactory.getLog(FlowStateDatabase.class);
	protected final int NO_AVOID_PORT = -1;
	protected final int WAIT_UNTIL_REMOVE_OBJECT = 23000;

	/**
	 * The FlowStateObjectGroup
	 */
	protected FlowStateObjectGroup flowStateObjectGroup = null;
	
	/**
	 * Bit to know if the FSDB has been modified
	 */
	protected boolean isModified = false;
	
	/*		Accessors		*/
	public boolean isModified()
	{
		return this.isModified;
	}
	public void setModified(boolean isModified)
	{
		this.isModified = isModified;
	}
	public FlowStateObjectGroup getFlowStateObjectGroup() {
		return flowStateObjectGroup;
	}

	/*		Constructors		*/
	public FlowStateDatabase()
	{
		this.flowStateObjectGroup = new FlowStateObjectGroup();
		this.isModified = false;
	}
	public boolean isEmpty()
	{
		return flowStateObjectGroup.getFlowStateObjectArray().size() == 0;
	}
	public byte[] encode (Encoder encoder) throws Exception
	{
		return encoder.encode(getFlowStateObjectGroup());
	}
	public void setAvoidPort(int avoidPort)
	{
		for (FlowStateObject fso: flowStateObjectGroup.getFlowStateObjectArray())
		{
			fso.setAvoidPort(avoidPort);
		}
	}
	
	public void addObjectToGroup(long address, int portId, long neighborAddress, int neighborPortId, FlowStateRIBObjectGroup fsRIBGroup )
	{
		FlowStateObject newObject = new FlowStateObject(address, portId, neighborAddress, neighborPortId, true, 1, 0);
		try {
			this.flowStateObjectGroup.getFlowStateObjectArray().add(newObject);
			fsRIBGroup.create(FlowStateObject.FLOW_STATE_RIB_OBJECT_CLASS, ObjectInstanceGenerator.getObjectInstance(), newObject.getID(), newObject);
			this.isModified = true;
		} catch (RIBDaemonException e) {
			log.debug("Error Code: " + e.getErrorCode());
			e.printStackTrace();
		}
	}
	
	/**
	 * Get the object given its port
	 * @param portId
	 * @return the flow state object
	 */
	private FlowStateObject getByPortId(int portId)
	{
		int i = 0;
		List<FlowStateObject> flowStateObjectArray = flowStateObjectGroup.getFlowStateObjectArray();
		while (i < flowStateObjectArray.size() && flowStateObjectArray.get(i).getPortid() != portId)
		{
			i++;
		}
		
		if (i == flowStateObjectArray.size())
		{
			return null;
		}
		else
		{
			return flowStateObjectArray.get(i);
		}
	}
	
	public boolean deprecateObject(int portId, int maximumAge)
	{
		FlowStateObject object = getByPortId(portId);
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
	
	private ArrayList<FlowStateObject> getModifiedFSO()
	{
		List<FlowStateObject> flowStateObjectArray = flowStateObjectGroup.getFlowStateObjectArray();
		ArrayList<FlowStateObject> modifiedFSOs = new ArrayList<FlowStateObject>();
		for(int i = 0; i< flowStateObjectArray.size(); i++)
		{
			if (flowStateObjectArray.get(i).isModified())
			{
				modifiedFSOs.add(flowStateObjectArray.get(i));
			}
		}
		return modifiedFSOs;
	}
	
	public ArrayList<FlowStateObjectGroup> prepareForPropagation(FlowInformation[] flows)
	{
		ArrayList<FlowStateObjectGroup> groupsToSend = new ArrayList<FlowStateObjectGroup>();
		if(!getModifiedFSO().isEmpty())
		{
			for(int i = 0; i < flows.length; i++)
			{
				groupsToSend.add(new FlowStateObjectGroup());
			}
		
			for(int i = 0; i < getModifiedFSO().size(); i++)
			{
				FlowStateObject object = getModifiedFSO().get(i);
				log.debug("Check modified object: " + object.getID() + " to be sent with age: " + object.getAge() +
						" and Status: " + object.isState());
	
				for(int j = 0; j < flows.length; j++)
				{
					int portId = flows[j].getPortId();
					if (object.getAvoidPort() != portId)
					{
						log.debug("To be sent to port: " + portId);
						groupsToSend.get(j).getFlowStateObjectArray().add(object);
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
		List<FlowStateObject> flowStateObjectArray = flowStateObjectGroup.getFlowStateObjectArray();
		for(int i = 0; i< flowStateObjectArray.size(); i++)
		{
			FlowStateObject object = flowStateObjectArray.get(i);
			object.setAge(object.getAge()+1);
			
			if (object.getAge() >= maximumAge && !object.isBeingErased())
			{
				log.debug("Object to erase age: " +object.getAge());
				Timer killFlowStateObjectTimer = new Timer();
				killFlowStateObjectTimer.schedule(new KillFlowStateObject(fsRIBGroup, flowStateObjectArray.get(i), this), 
						WAIT_UNTIL_REMOVE_OBJECT);
				object.setBeingErased(true);
			}
		}
	}
	
	public void updateObjects(FlowStateObjectGroup newObjectGroup, int avoidPort, FlowStateRIBObjectGroup fsRIBGroup, long address)
	{
		log.debug("Update Objects from DB launched");
		
		List<FlowStateObject> newObjects= newObjectGroup.getFlowStateObjectArray();
		List<FlowStateObject> oldObjects = this.flowStateObjectGroup.getFlowStateObjectArray();
		boolean continueLoop = true;
		int i = 0;
		
		for(FlowStateObject newObj : newObjects)
		{
			while (continueLoop && i < oldObjects.size())
			{
				FlowStateObject oldObj = oldObjects.get(i);
				
				if (newObj.getAddress() == oldObj.getAddress() /*&& objM.getPortid() == obj.getPortid()*/
						&& newObj.getNeighborAddress() == oldObj.getNeighborAddress() 
						/*&& objM.getNeighborPortid() == obj.getNeighborPortid()*/)
				{
					log.debug("Found the object in the DB. Obj: " + newObj.getID());
					continueLoop = false;
					if (newObj.getSequenceNumber() > oldObj.getSequenceNumber())
					{
						if (newObj.getAddress() == address)
						{
							oldObj.setSequenceNumber(newObj.getSequenceNumber() + 1);
							oldObj.setAvoidPort(NO_AVOID_PORT);
							log.debug("Object is self generated, updating sequence number of " + oldObj.getID() + " to seqnum: " 
									+ newObj.getSequenceNumber());
						}
						else
						{
							log.debug("Update the object: " + oldObj.getID() + " with seq num: " + oldObj.getSequenceNumber());
							oldObj.setAge(newObj.getAge());
							oldObj.setState(newObj.isState());
							oldObj.setSequenceNumber(newObj.getSequenceNumber());
							oldObj.setAvoidPort(avoidPort);
						}
						oldObj.setModified(true);
						this.isModified = true;
					}
				}
				
				i++;
			}
			if (continueLoop)
			{
				if (newObj.getAddress() == address)
				{
					log.debug("Object has orgin myself, discart object " + newObj.getID());
				}
				else
				{
					log.debug("New object added");
					newObj.setAvoidPort(avoidPort);
					newObj.setModified(true);
					try {
						flowStateObjectGroup.getFlowStateObjectArray().add(newObj);
						fsRIBGroup.create(FlowStateObject.FLOW_STATE_RIB_OBJECT_CLASS, ObjectInstanceGenerator.getObjectInstance(), newObj.getID(), newObj);

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
