package rina.ipcprocess.impl.PDUForwardingTable.routingalgorithms.dijkstra;

import java.util.ArrayList;

import rina.PDUForwardingTable.api.VertexInt;

public class Edge  {
	  private final VertexInt v1;
	  private final VertexInt v2;
	  private final int weight;
//	  private int portV1;
//	  private int portV2;
	  
	  public Edge(VertexInt v1, VertexInt v2, int weight) {
	    this.v1 = v1;
//	    this.portV1 = portV1;
//	    this.portV2 = portV2;
	    this.v2 = v2;
	    this.weight = weight;
	  }

//	  public int getPortV1() {
//		return portV1;
//	  }
//
//	  public void setPortV1(int portV1) {
//		this.portV1 = portV1;
//	  }
//
//	  public int getPortV2() {
//		return portV2;
//	  }
//
//	  public void setPortV2(int portV2) {
//		this.portV2 = portV2;
//	  }

	  public VertexInt getV2() {
	    return v2;
	  }

	  public VertexInt getV1() {
	    return v1;
	  }
	  public int getWeight() {
	    return weight;
	  }
	  
	  public boolean isVertexIn(VertexInt v)
	  {
		  if (v.equals(v1) || v.equals(v2))
		  {
			  return true;
		  }
		  else
		  {
			  return false;
		  }
	  }
	  
	  public VertexInt getOtherEndpoint(VertexInt v)
	  {
		  if (v.equals(v1))
		  {
			  return v2;
		  }
		  else
		  {
			  if (v.equals(v2))
			  {
				  return v1;
			  }
			  else
			  {
				  // TODO: Throw exception
				  return null;
			  }
		  }
	  }
	  
	  public ArrayList<VertexInt> getEndpoints()
	  {
		  ArrayList<VertexInt> array = new ArrayList<VertexInt>();
		  array.add(v1);
		  array.add(v2);
		  return array;
	  }
	  
	  @Override
	  public boolean equals(Object obj) {
		    if (this == obj)
		      return true;
		    if (obj == null)
		      return false;
		    if (getClass() != obj.getClass())
		      return false;
		    Edge other = (Edge) obj;
		    if (!other.getEndpoints().contains(v1) || other.getOtherEndpoint(v1) != v2 || weight != other.weight) 
//		    		|| portV1 != other.portV1 || portV2 != other.portV2)
		      return false;
		    return true;
		  }	  
	  @Override
	  public String toString() {
	    return v1 + " " + v2;
	  }
	}

