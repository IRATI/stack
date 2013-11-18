package rina.ribdaemon.api;

/**
 * A Simple RIB object that is a member of a set
 * (for example whatevercast names, qos cubes, ...).
 * As with the Simple RIB Object, the RIB Object is 
 * just a container for the data
 * @author eduardgrasa
 *
 */
public class SimpleSetMemberRIBObject extends SimpleRIBObject{

	public SimpleSetMemberRIBObject(String objectClass,
			String objectName, Object value) {
		super(objectClass, objectName, value);
	}

	@Override
	public void delete(Object objectValue) throws RIBDaemonException {
		this.getParent().removeChild(this.getObjectName());
		this.getRIBDaemon().removeRIBObject(this);
	}
}
