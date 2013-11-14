package rina.ipcprocess.impl.enrollment.ribobjects;

import eu.irati.librina.Neighbor;
import rina.ribdaemon.api.BaseRIBObject;
import rina.ribdaemon.api.ObjectInstanceGenerator;
import rina.ribdaemon.api.RIBDaemonException;
import rina.ribdaemon.api.RIBObject;

public class NeighborRIBObject extends BaseRIBObject{
    
    public static final String NEIGHBOR_RIB_OBJECT_CLASS = "neighbor";
    
	private Neighbor neighbor = null;
	
	public NeighborRIBObject(String objectName, Neighbor neighbor) {
		super(NEIGHBOR_RIB_OBJECT_CLASS, 
				ObjectInstanceGenerator.getObjectInstance(), objectName);
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
			this.neighbor = (Neighbor) object;
		}
	}
	
	@Override
	public synchronized void delete(Object object) throws RIBDaemonException {
		this.getParent().removeChild(this.getObjectName());
	}
	
	@Override
	public synchronized Object getObjectValue(){
		return neighbor;
	}
}
