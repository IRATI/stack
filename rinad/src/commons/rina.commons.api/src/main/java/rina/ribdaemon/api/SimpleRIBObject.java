package rina.ribdaemon.api;

import rina.ipcprocess.api.IPCProcess;

/**
 * A simple RIB object that just acts as a wrapper. Represents an object in the RIB that just 
 * can be read or written, and whose read/write operations have no side effects other than 
 * updating the value of the object
 * for an object value
 * @author eduardgrasa
 *
 */
public class SimpleRIBObject extends BaseRIBObject{

	private Object objectValue = null;
	
	public SimpleRIBObject(IPCProcess ipcProcess, String objectClass, String objectName, Object value) {
		super(ipcProcess, objectClass, ObjectInstanceGenerator.getObjectInstance(), objectName);
		this.objectValue = value;
	}
	
	@Override
	public RIBObject read() throws RIBDaemonException{
		return this;
	}
	
	@Override
	public synchronized void write(Object value) throws RIBDaemonException {
		this.objectValue = value;
	}

	/**
	 * In this case create has the semantics of update 
	 */
	@Override
	public synchronized void create(String objectClass, long objectInstance, String objectName, Object value) throws RIBDaemonException{
		this.objectValue = value;
	}

	@Override
	public synchronized Object getObjectValue() {
		return objectValue;
	}

}
