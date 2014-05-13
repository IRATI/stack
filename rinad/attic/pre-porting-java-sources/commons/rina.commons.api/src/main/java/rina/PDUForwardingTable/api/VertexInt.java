package rina.PDUForwardingTable.api;

public interface VertexInt {
	public long getAddress();
	
	@Override
	public int hashCode();
	
	@Override
	public boolean equals(Object obj);
}
