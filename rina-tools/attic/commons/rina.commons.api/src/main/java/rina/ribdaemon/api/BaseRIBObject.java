package rina.ribdaemon.api;

import java.util.ArrayList;
import java.util.List;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import rina.cdap.api.CDAPSessionDescriptor;
import rina.cdap.api.message.CDAPMessage;
import rina.encoding.api.Encoder;
import rina.ipcprocess.api.IPCProcess;

/**
 * The base RIB Handler that provides a default behaviour for the RIB Handlers. 
 * The default behaviour consists in throwing exceptions because the operation 
 * is not supported and returning CDAP Error messages for the same reason. The 
 * error code for the operation not supported is "1".
 * Subclasses just need to overwrite the methods that are allowed in a certain RIB 
 * object.
 * It also provides some basic machinery that will be required to encode/decode 
 * the CDAP message contents and a pointer to the RIB Daemon.
 * @author eduardgrasa
 *
 */
public abstract class BaseRIBObject implements RIBObject{

	private static final Log log = LogFactory.getLog(BaseRIBObject.class);
	
	private Encoder encoder = null;
	private RIBDaemon ribDaemon = null;

	/**
	 * The attributes of this object: objectName, objectClass, objectInstance, objectValue
	 */
	private String objectName = " ";
	private String objectClass = " ";
	private long objectInstance = 0;

	/**
	 * The list of children of this object in the RIB
	 */
	private List<RIBObject> children = null;

	/**
	 * The parent object of this object in the RIB
	 */
	private RIBObject parent = null;

	/**
	 * The permissions associated to this RIB node
	 */
	private Permissions permissions = null;
	
	private IPCProcess ipcProcess = null;

	public BaseRIBObject(IPCProcess ipcProcess, String objectClass, long objectInstance, String objectName) {
		this.ipcProcess = ipcProcess;
		children = new ArrayList<RIBObject>();
		this.objectName = objectName;
		this.objectClass = objectClass;
		this.objectInstance = objectInstance;
		this.ribDaemon = ipcProcess.getRIBDaemon();
	}
	
	public IPCProcess getIPCProcess() {
		return ipcProcess;
	}
	
	public eu.irati.librina.RIBObject toLibrinaRIBObject() {
		eu.irati.librina.RIBObject result = new eu.irati.librina.RIBObject();
		result.setName(objectName);
		result.setClazz(objectClass);
		result.setInstance(objectInstance);
		if (getObjectValue() != null) {
			result.setDisplayableValue(getObjectValue().toString());
		}
		
		return result;
	}

	public Permissions getPermissions() {
		return permissions;
	}

	public void setPermissions(Permissions permissions) {
		this.permissions = permissions;
	}

	public String getObjectName(){
		return this.objectName;
	}

	public String getObjectClass(){
		return this.objectClass;
	}

	public long getObjectInstance(){
		return this.objectInstance;
	}

	public RIBObject getParent(){
		return this.parent;
	}

	public void setParent(RIBObject parent){
		this.parent = parent;
	}

	public abstract Object getObjectValue();

	/**
	 * Return the children of RIBObject. The RIB is represented by a single
	 * root RIBObject whose children are represented by a List<RIBObject>. Each of
	 * these RIBObject elements in the List can have children. The getChildren()
	 * method will return the children of a RIBObject.
	 * @return the children of RIBObject
	 */
	public List<RIBObject> getChildren() {
		return this.children;
	}

	/**
	 * Sets the children of a RIBObject object. See docs for getChildren() for
	 * more information.
	 * @param children the List<RIBObject> to set.
	 */
	public void setChildren(List<RIBObject> children) {
		this.children = children;
	}

	/**
	 * Returns the number of immediate children of this RIBNode.
	 * @return the number of immediate children.
	 */
	public int getNumberOfChildren() {
		return children.size();
	}

	/**
	 * Adds a child to the list of children for this RIBObject. The addition of
	 * the first child will create a new List<RIBObject>.
	 * @param child a RIB object to set.
	 */
	public void addChild(RIBObject child) throws RIBDaemonException{
		for(int i=0; i<children.size(); i++){
			if (children.get(i).getObjectName().equals(child.getObjectName())){
				throw new RIBDaemonException(RIBDaemonException.OBJECT_ALREADY_HAS_THIS_CHILD);
			}
		}

		children.add(child);
		child.setParent(this);
	}

	public void removeChild(String objectName) throws RIBDaemonException{
		RIBObject child = null;

		for(int i= 0; i<children.size(); i++){
			child = children.get(i);
			if (child.getObjectName().equals(objectName)){
				log.debug("child found, removing");
				children.remove(child);
				child.setParent(null);
				return;
			}
		}
		throw new RIBDaemonException(RIBDaemonException.CHILD_NOT_FOUND);
	}

	public boolean hasChild(String objectName){
		for(int i=0; i<children.size(); i++){
			if (children.get(i).getObjectName().equals(objectName)){
				return true;
			}
		}

		return false;
	}

	public RIBObject getChild(String objectName){
		for(int i=0; i<children.size(); i++){
			if (children.get(i).getObjectName().equals(objectName)){
				return children.get(i);
			}
		}

		return null;
	}

	public void setEncoder(Encoder encoder) {
		this.encoder = encoder;
	}

	public void setRIBDaemon(RIBDaemon ribDaemon) {
		this.ribDaemon = ribDaemon;
	}

	public Encoder getEncoder(){
		return this.encoder;
	}

	public RIBDaemon getRIBDaemon(){
		return this.ribDaemon;
	}

	public void create(String objectClass, long objectInstance, String objectName, Object objectValue) throws RIBDaemonException{
		throw new RIBDaemonException(RIBDaemonException.OPERATION_NOT_ALLOWED_AT_THIS_OBJECT, 
				"Operation CREATE not allowed for objectName "+objectName);
	}

	public void delete(Object object) throws RIBDaemonException {
		throw new RIBDaemonException(RIBDaemonException.OPERATION_NOT_ALLOWED_AT_THIS_OBJECT, 
				"Operation DELETE not allowed for objectName "+objectName);
	}

	public RIBObject read() throws RIBDaemonException {
		throw new RIBDaemonException(RIBDaemonException.OPERATION_NOT_ALLOWED_AT_THIS_OBJECT, 
				"Operation READ not allowed for objectName "+objectName);
	}

	public void write(Object object) throws RIBDaemonException {
		throw new RIBDaemonException(RIBDaemonException.OPERATION_NOT_ALLOWED_AT_THIS_OBJECT, 
				"Operation WRITE not allowed for objectName "+objectName);
	}

	public void start(Object object) throws RIBDaemonException {
		throw new RIBDaemonException(RIBDaemonException.OPERATION_NOT_ALLOWED_AT_THIS_OBJECT, 
				"Operation START not allowed for objectName "+objectName);
	}

	public void stop(Object object) throws RIBDaemonException {
		throw new RIBDaemonException(RIBDaemonException.OPERATION_NOT_ALLOWED_AT_THIS_OBJECT, 
				"Operation STOP not allowed for objectName "+objectName);
	}

	public void read(CDAPMessage cdapMessage, CDAPSessionDescriptor cdapSessionDescriptor) throws RIBDaemonException {
	}

	public void write(CDAPMessage cdapMessage, CDAPSessionDescriptor cdapSessionDescriptor) throws RIBDaemonException {
	}

	public void create(CDAPMessage cdapMessage, CDAPSessionDescriptor cdapSessionDescriptor) throws RIBDaemonException {
	}

	public void delete(CDAPMessage cdapMessage, CDAPSessionDescriptor cdapSessionDescriptor) throws RIBDaemonException {
	}

	public void cancelRead(CDAPMessage cdapMessage, CDAPSessionDescriptor cdapSessionDescriptor) throws RIBDaemonException {
	}

	public void start(CDAPMessage cdapMessage, CDAPSessionDescriptor cdapSessionDescriptor) throws RIBDaemonException {
	}

	public void stop(CDAPMessage cdapMessage, CDAPSessionDescriptor cdapSessionDescriptor) throws RIBDaemonException {
	}
}
