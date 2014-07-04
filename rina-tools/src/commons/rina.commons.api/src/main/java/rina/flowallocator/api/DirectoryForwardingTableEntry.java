package rina.flowallocator.api;

import eu.irati.librina.ApplicationProcessNamingInformation;

/**
 * An entry of the directory forwarding table
 * @author eduardgrasa
 */
public class DirectoryForwardingTableEntry {
	
	/**
	 * The name of the application process
	 */
	private ApplicationProcessNamingInformation apNamingInfo = null;
	
	/**
	 * The address of the IPC process it is currently attached to
	 */
	private long address = 0L;
	
	/**
	 * A timestamp for this entry
	 */
	private long timestamp = 0L;
	
	public DirectoryForwardingTableEntry(){
	}
	
	public DirectoryForwardingTableEntry(
			ApplicationProcessNamingInformation apNamingInfo, long address, long timestamp){
		this.apNamingInfo = apNamingInfo;
		this.address = address;
		this.timestamp = timestamp;
	}

	public ApplicationProcessNamingInformation getApNamingInfo() {
		return apNamingInfo;
	}

	public void setApNamingInfo(ApplicationProcessNamingInformation apNamingInfo) {
		this.apNamingInfo = apNamingInfo;
	}

	public long getAddress() {
		return address;
	}

	public void setAddress(long address) {
		this.address = address;
	}

	public long getTimestamp() {
		return timestamp;
	}

	public void setTimestamp(long timestamp) {
		this.timestamp = timestamp;
	}

	/**
	 * Returns a key identifying this entry
	 * @return
	 */
	public String getKey(){
		return this.apNamingInfo.getEncodedString();
	}
	
	@Override
	public boolean equals(Object object){
		if (object == null){
			return false;
		}
		
		if (!(object instanceof DirectoryForwardingTableEntry)){
			return false;
		}
		
		DirectoryForwardingTableEntry candidate = (DirectoryForwardingTableEntry) object;
		
		if (candidate.getApNamingInfo() == null || !candidate.getApNamingInfo().equals(this.getApNamingInfo())){
			return false;
		}
		
		if (candidate.getAddress() != this.getAddress()){
			return false;
		}
		
		return true;
	}
	
	@Override
	public String toString(){
		String result = this.apNamingInfo.toString();
		result = result + "IPC Process address: " + this.getAddress() + "\n";
		result = result + "Timestamp: " + this.getTimestamp() + "\n";
		
		return result;
	}
}
