package rina.ipcprocess.impl.PDUForwardingTable.internalobjects;

import rina.ribdaemon.api.RIBObjectNames;

public class FlowStateInternalObject{
	
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

	/* Object has been modified */
	protected boolean isModified;
	
	protected int avoidPort;
	
	/*		Acessors	*/
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
	public void setNeighborPortid(int neighbor_portid) {
		this.neighborPortid = neighbor_portid;
	}
	public int getSequenceNumber() {
		return sequenceNumber;
	}
	public void setSequenceNumber(int sequenceNumber) {
		this.sequenceNumber = sequenceNumber;
	}
	public boolean isState() {
		return state;
	}
	public void setModified(boolean isModified) {
		this.isModified = isModified;
	}
	public int getAge() {
		return age;
	}
	public void setAge(int age) {
		this.age = age;
	}
	public boolean isModified() {
		return isModified;
	}
	public void setState(boolean state) {
		this.state = state;
	}
	public int getAvoidPort() {
		return avoidPort;
	}
	public void setAvoidPort(int avoidPort) {
		this.avoidPort = avoidPort;
	}
	public String getID()
	{
		return this.ID;
	}
	
	/*		Constructor	*/
	public FlowStateInternalObject(long address, int portid, long neighbor_address, int neighbor_portid, boolean state, int sequence_number, int age)
	{
		this.address = address;
		this.portid = portid;
		this.neighborAddress = neighbor_address;
		this.neighborPortid = neighbor_portid;
		this.state  = state;
		this.sequenceNumber = sequence_number;
		this.age = age;
		this.isModified = true;
		this.avoidPort = -1;
		this.ID = this.ID + address + portid + neighborAddress + neighborPortid;
	}
	public FlowStateInternalObject(FlowStateInternalObject obj)
	{
		this.address = obj.getAddress();
		this.portid = obj.getPortid();
		this.neighborAddress = obj.getNeighborAddress();
		this.neighborPortid = obj.getNeighborPortid();
		this.state = obj.isState();
		this.sequenceNumber = obj.getSequenceNumber();
		this.age = obj.getAge();
		this.isModified = obj.isModified();
		this.avoidPort = obj.getAvoidPort();
		this.ID = this.ID + address + portid + neighborAddress + neighborPortid;
	}
	
	public void incrementAge()
	{
		this.age++;
	}
}
