package rina.PDUForwardingTable.api;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import rina.ribdaemon.api.RIBObjectNames;

/**
 * @author bernat
 *
 */
public class FlowStateObject{
	
	private static final Log log = LogFactory.getLog(FlowStateObject.class);
	
	public static final String FLOW_STATE_RIB_OBJECT_CLASS = "flowstateobject";
	
	protected String ID = RIBObjectNames.SEPARATOR + 
			RIBObjectNames.DIF + RIBObjectNames.SEPARATOR + RIBObjectNames.MANAGEMENT
			+ RIBObjectNames.SEPARATOR + RIBObjectNames.ROUTING + RIBObjectNames.SEPARATOR 
			+ RIBObjectNames.FLOWSTATEOBJECTGROUP + RIBObjectNames.SEPARATOR;
	
	/* The address of the IPC Process */ 
	protected long address;
	
	/* The port-id of the N-1 flow */
	protected int portid;
	
	/* The address of the neighbor IPC Process */ 
	protected long neighborAddress; 

	/* The port_id assigned by the neighbor IPC Process to the N-1 flow */
	protected int neighborPortid;
	
	/* Flow up (true) or down (false) */ 
	protected boolean state;
	
	/* A sequence number to be able to discard old information */
	protected int sequenceNumber;
	
	/* Age of this FSO (in seconds) */
	protected int age;
	
	/* The object has been marked for propagation */
	protected boolean isModified;
	
	/* The object is being erased */
	protected boolean isBeingErased;
		
	/*		Accessors		*/
	public long getAddress() {
		return address;
	}
	public void setAddress(long address) {
		this.address = address;
	}
	public int getPortid() {
		return portid;
	}
	public void setPortid(int portid) {
		this.portid = portid;
	}
	public long getNeighborAddress() {
		return neighborAddress;
	}
	public void setNeighborAddress(long neighborAddress) {
		this.neighborAddress = neighborAddress;
	}
	public int getNeighborPortid() {
		return neighborPortid;
	}
	public void setNeighborPortid(int neighborPortid) {
		this.neighborPortid = neighborPortid;
	}
	public boolean isState() {
		return state;
	}
	public void setState(boolean state) {
		this.state = state;
	}
	public int getSequenceNumber() {
		return sequenceNumber;
	}
	public void setSequenceNumber(int sequenceNumber) {
		this.sequenceNumber = sequenceNumber;
	}
	public int getAge() {
		return age;
	}
	public void setAge(int age) {
		this.age = age;
	}
	public String getID()
	{
		return this.ID;
	}
	public boolean isModified() {
		return isModified;
	}
	public void setModified(boolean isModified) {
		this.isModified = isModified;
	}
	public boolean isBeingErased() {
		return isBeingErased;
	}
	public void setBeingErased(boolean isBeingErased) {
		this.isBeingErased = isBeingErased;
	}
	
	/*		Constructor	*/
	public FlowStateObject(long address, int portid, long neighborAddress, int neighborPortid, boolean state, int sequenceNumber, int age)
	{
		this.address = address;
		this.portid = portid;
		this.neighborAddress = neighborAddress;
		this.neighborPortid = neighborPortid;
		this.state  = state;
		this.sequenceNumber = sequenceNumber;
		this.age = age;
		this.ID = this.ID + address + /*portid +*/ neighborAddress /*+ neighborPortid*/;
		this.isModified = false;
		this.isBeingErased = false;
		log.debug("Created object with id: " + this.ID);
	}
	
	public String toString() {
		String result = "";
		result = result + "Address: " + address + "; Neighbor address: "+neighborAddress + "\n";
		result = result + "Port-id: " + portid + "; Neighbor port-id: " + neighborPortid + "\n";
		result = result + "State: " + state + "; Sequence number: " + sequenceNumber +
				"; Age: "+ age;
		
		return result;
	}
}
