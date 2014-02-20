package rina.ipcprocess.impl.enrollment.ribobjects;

import eu.irati.librina.Neighbor;
import rina.ipcprocess.api.IPCProcess;
import rina.ribdaemon.api.BaseRIBObject;
import rina.ribdaemon.api.ObjectInstanceGenerator;
import rina.ribdaemon.api.RIBDaemonException;
import rina.ribdaemon.api.RIBObject;

public class NeighborRIBObject extends BaseRIBObject{
    
    public static final String NEIGHBOR_RIB_OBJECT_CLASS = "neighbor";
   
	private Neighbor neighbor = null;
	
	public NeighborRIBObject(IPCProcess ipcProcess, String objectName, Neighbor neighbor) {
		super(ipcProcess, NEIGHBOR_RIB_OBJECT_CLASS, 
				ObjectInstanceGenerator.getObjectInstance(), objectName);
		setRIBDaemon(ipcProcess.getRIBDaemon());
		this.neighbor = neighbor;
	}
	
	@Override
	public RIBObject read() throws RIBDaemonException{
		return this;
	}
	
	@Override
	public void create(String objectClass, long objectInstance, String objectName, Object objectValue) throws RIBDaemonException{
		write(objectValue);
	}
	
	@Override
	public synchronized void write(Object object) throws RIBDaemonException {
		if (!(object instanceof Neighbor)){
			throw new RIBDaemonException(RIBDaemonException.OBJECTCLASS_DOES_NOT_MATCH_OBJECTNAME, 
					"Object class ("+object.getClass().getName()+") does not match object name "+
							this.getObjectName());
		}

		Neighbor newNeighbor = (Neighbor) object;
		neighbor.setAddress(newNeighbor.getAddress());
		neighbor.setAverageRttInMs(newNeighbor.getAverageRttInMs());
		neighbor.setLastHeardFromTimeInMs(newNeighbor.getLastHeardFromTimeInMs());
		neighbor.setName(newNeighbor.getName());
		neighbor.setSupportingDifName(newNeighbor.getSupportingDifName());
		neighbor.setUnderlyingPortId(newNeighbor.getUnderlyingPortId());
	    neighbor.setEnrolled(newNeighbor.isEnrolled());
	}
	
	@Override
	public synchronized void delete(Object object) throws RIBDaemonException {
		getParent().removeChild(this.getObjectName());
		getRIBDaemon().removeRIBObject(this);
	}
	
	@Override
	public synchronized Object getObjectValue(){
		return neighbor;
	}
}
