package rina.ipcprocess.impl.PDUForwardingTable.routingalgorithms.dijkstra;

import rina.ipcprocess.impl.PDUForwardingTable.routingalgorithms.AbstractVertex;


public class Vertex extends AbstractVertex{
	  

	  public Vertex(long address, int portId)
	  {
		  super(address, portId);
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
	    if (address != other.getAddress() || this.portId != other.getPortId())
	      return false;
	    return true;
	  }	  
	}
