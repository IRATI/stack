package rina.ipcprocess.impl.PDUForwardingTable.routingalgorithms;

public abstract class AbstractVertex implements VertexInt{
	
	  protected final long address;
	  protected final int portId;
	  
	  public AbstractVertex(long address, int portId) {
		    this.address = address;
		    this.portId = portId;
		  }  
	  public long getAddress() {
		    return address;
		  }

	  public int getPortId() {
	    return portId;
	  }
}
