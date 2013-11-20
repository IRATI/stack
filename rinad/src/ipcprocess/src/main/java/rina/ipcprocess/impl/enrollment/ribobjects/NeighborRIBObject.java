package rina.ipcprocess.impl.enrollment.ribobjects;

import eu.irati.librina.Neighbor;
import rina.enrollment.api.EnrollmentTask;
import rina.ipcprocess.impl.IPCProcess;
import rina.ribdaemon.api.BaseRIBObject;
import rina.ribdaemon.api.ObjectInstanceGenerator;
import rina.ribdaemon.api.RIBDaemonException;
import rina.ribdaemon.api.RIBObject;

public class NeighborRIBObject extends BaseRIBObject{
    
    public static final String NEIGHBOR_RIB_OBJECT_CLASS = "neighbor";
    
    private EnrollmentTask enrollmentTask = null;
	private Neighbor neighbor = null;
	
	public NeighborRIBObject(String objectName, Neighbor neighbor) {
		super(NEIGHBOR_RIB_OBJECT_CLASS, 
				ObjectInstanceGenerator.getObjectInstance(), objectName);
		setRIBDaemon(IPCProcess.getInstance().getRIBDaemon());
		enrollmentTask = IPCProcess.getInstance().getEnrollmentTask();
		enrollmentTask.addNeighbor(neighbor);
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
	public void write(Object object) throws RIBDaemonException {
		if (!(object instanceof Neighbor)){
			throw new RIBDaemonException(RIBDaemonException.OBJECTCLASS_DOES_NOT_MATCH_OBJECTNAME, 
					"Object class ("+object.getClass().getName()+") does not match object name "+
					this.getObjectName());
		}
		
		synchronized(this){
			if (this.neighbor == null) {
				enrollmentTask.addNeighbor(neighbor);
				this.neighbor = (Neighbor) object;
			} else {
				this.neighbor.setAddress(this.neighbor.getAddress());
				this.neighbor.setAverageRttInMs(this.neighbor.getAverageRttInMs());
				this.neighbor.setLastHeardFromTimeInMs(this.neighbor.getLastHeardFromTimeInMs());
				this.neighbor.setName(this.neighbor.getName());
				this.neighbor.setSupportingDifName(this.neighbor.getSupportingDifName());
				this.neighbor.setUnderlyingPortId(this.neighbor.getUnderlyingPortId());
			}
		}
	}
	
	@Override
	public synchronized void delete(Object object) throws RIBDaemonException {
		this.enrollmentTask.removeNeighbor(neighbor);
		this.getParent().removeChild(this.getObjectName());
	}
	
	@Override
	public synchronized Object getObjectValue(){
		return neighbor;
	}
}
