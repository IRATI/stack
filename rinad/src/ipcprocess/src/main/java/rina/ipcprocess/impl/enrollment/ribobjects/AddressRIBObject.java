package rina.ipcprocess.impl.enrollment.ribobjects;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import rina.ipcprocess.impl.IPCProcess;
import rina.ribdaemon.api.BaseRIBObject;
import rina.ribdaemon.api.ObjectInstanceGenerator;
import rina.ribdaemon.api.RIBDaemonException;
import rina.ribdaemon.api.RIBObject;
import rina.ribdaemon.api.RIBObjectNames;

/**
 * Handles the operations related to the "daf.management.naming.currentsynonym" objects
 * @author eduardgrasa
 *
 */
public class AddressRIBObject extends BaseRIBObject{
	
	public static final String ADDRESS_RIB_OBJECT_NAME = RIBObjectNames.SEPARATOR + RIBObjectNames.DAF + 
			RIBObjectNames.SEPARATOR + RIBObjectNames.MANAGEMENT + RIBObjectNames.SEPARATOR + RIBObjectNames.NAMING + 
			RIBObjectNames.SEPARATOR + RIBObjectNames.ADDRESS;

	private static final Log log = LogFactory.getLog(AddressRIBObject.class);
	
	public static final String ADDRESS_RIB_OBJECT_CLASS = "address";

	private IPCProcess ipcProcess = null;

	public AddressRIBObject(){
		super(ADDRESS_RIB_OBJECT_CLASS, ObjectInstanceGenerator.getObjectInstance(), 
				ADDRESS_RIB_OBJECT_NAME);
		ipcProcess = IPCProcess.getInstance();
	}
	
	@Override
	public RIBObject read() throws RIBDaemonException{
		return this;
	}

	@Override
	public void write(Object object) throws RIBDaemonException {
		if (!(object instanceof Long)){
			throw new RIBDaemonException(RIBDaemonException.OBJECTCLASS_DOES_NOT_MATCH_OBJECTNAME, 
					"Object class ("+object.getClass().getName()+") does not match object name "+this.getObjectName());
		}
		
		if (ipcProcess.getDIFInformation() == null) {
			log.warn("Tried to wite address, but DIF information is NULL, shouldn't have happened");
		} else {
			ipcProcess.getDIFInformation().getDifConfiguration().setAddress(
					((Long)object).longValue());
		}
		
		
	}
	
	@Override
	public synchronized Object getObjectValue(){
		return ipcProcess.getAddress();
	}

}
