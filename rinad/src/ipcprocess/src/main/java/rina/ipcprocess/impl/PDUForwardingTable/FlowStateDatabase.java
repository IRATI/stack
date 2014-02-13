package rina.ipcprocess.impl.PDUForwardingTable;

import java.util.ArrayList;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import rina.PDUForwardingTable.api.FlowStateObject;
import rina.ipcprocess.impl.PDUForwardingTable.internalobjects.FlowStateInternalObject;
import rina.ipcprocess.impl.PDUForwardingTable.internalobjects.FlowStateInternalObjectGroup;
import rina.ipcprocess.impl.PDUForwardingTable.internalobjects.ObjectStateMapper;
import rina.ipcprocess.impl.PDUForwardingTable.ribobjects.FlowStateRIBObjectGroup;
import rina.ribdaemon.api.ObjectInstanceGenerator;
import rina.ribdaemon.api.RIBDaemonException;

public class FlowStateDatabase {
	private static final Log log = LogFactory.getLog(FlowStateDatabase.class);
	/**
	 * The FlowStateObjectGroup
	 */
	protected FlowStateInternalObjectGroup flowStateObjectGroup = null;
	
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
	public FlowStateInternalObjectGroup getFlowStateObjectGroup() {
		return flowStateObjectGroup;
	}
	/*		Constructors		*/
	public FlowStateDatabase()
	{
		this.flowStateObjectGroup = new FlowStateInternalObjectGroup();
		this.isModified = false;
	}
	
	/*		Methods		*/
	public void addObjectToGroup(FlowStateInternalObject internalObject, FlowStateRIBObjectGroup fsRIBGroup )
	{
		try {
			this.flowStateObjectGroup.add(internalObject, fsRIBGroup);
			this.isModified = true;
		} catch (RIBDaemonException e) {
			e.printStackTrace();
		}

	}
	
	public FlowStateInternalObject getByPortId(int portId)
	{
		return this.flowStateObjectGroup.getByPortId(portId);
	}
	
	public ArrayList<FlowStateInternalObject> getModifiedFSO()
	{
		return this.flowStateObjectGroup.getModifiedFSO();
	}
	

	public void incrementAge(int maximumAge)
	{
		boolean modified = this.flowStateObjectGroup.incrementAge(maximumAge);
		if (modified)
		{
			this.isModified = true;
		}
	}
	
	public void updateObjects(FlowStateInternalObjectGroup groupToModify, int avoidPort, FlowStateRIBObjectGroup fsRIBGroup)
	{
		log.debug("Update Objects from DB launched");
		ArrayList<FlowStateInternalObject> objectsToModify= groupToModify.getFlowStateObjectArray();
		ArrayList<FlowStateInternalObject> objects = this.flowStateObjectGroup.getFlowStateObjectArray();
		boolean continueLoop = true;
		int i = 0;
		
		for(FlowStateInternalObject objM : objectsToModify)
		{
			while (continueLoop && i < objects.size())
			{
				FlowStateInternalObject obj = objects.get(i);
				
				if (objM.getAddress() == obj.getAddress() /*&& objM.getPortid() == obj.getPortid()*/
						&& objM.getNeighborAddress() == obj.getNeighborAddress() 
						/*&& objM.getNeighborPortid() == obj.getNeighborPortid()*/)
				{
					continueLoop = false;
					if (objM.getSequenceNumber() > obj.getSequenceNumber())
					{
						obj.setState(objM.isState());
						obj.setSequenceNumber(objM.getSequenceNumber());
						obj.setModified(true);
						obj.setAvoidPort(avoidPort);
						
						this.isModified = true;
					}
				}
				
				i++;
			}
			if (continueLoop)
			{
				log.debug("New object added");
				objM.setAvoidPort(avoidPort);
				objM.setModified(true);
				try {
					this.flowStateObjectGroup.add(objM, fsRIBGroup);
				} catch (RIBDaemonException e) {
					log.error("could not add the object to the rib");
					e.printStackTrace();
				}

				this.isModified = true;
			}
			continueLoop = true;
			i = 0;
		}
	}

}
