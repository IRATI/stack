package rina.ipcprocess.impl.PDUForwardingTable.routingalgorithms.dijkstra;

import rina.ipcprocess.impl.PDUForwardingTable.routingalgorithms.VertexInt;


public class Vertex implements VertexInt{
	  
	protected final long address;
	
	public Vertex(long address)
	{
		this.address = address;
	}
	
	public long getAddress() 
	{
		return address;
	}
	  
	@Override
	public int hashCode() {
		final int prime = 31;
		int result = 1;
		result = prime * result + (int)address;
		return result;
	}
	  
	@Override
	public boolean equals(Object obj) {
		if (this == obj)
		  return true;
		if (obj == null)
		  return false;
		if (getClass() != obj.getClass())
		  return false;
		Vertex other = (Vertex) obj;
		if (address != other.getAddress())
		  return false;
		return true;
	}	  
}
