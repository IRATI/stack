package rina.pduftg.api.linkstate;

public interface VertexInt {
	public long getAddress();
	
	@Override
	public int hashCode();
	
	@Override
	public boolean equals(Object obj);
}
