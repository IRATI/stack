package rina.ipcprocess.impl.pduftg.linkstate.routingalgorithms.dijkstra;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.Set;

import rina.pduftg.api.linkstate.FlowStateObject;
import rina.pduftg.api.linkstate.RoutingAlgorithmInt;
import rina.pduftg.api.linkstate.VertexInt;

import eu.irati.librina.PDUForwardingTableEntry;
import eu.irati.librina.PDUForwardingTableEntryList;

public class DijkstraAlgorithm implements RoutingAlgorithmInt{
	public class PredecessorInfo
	{
		public VertexInt predecessor;
		public int portId;
		
		public PredecessorInfo(VertexInt nPredecessor, Edge edge)
		{
			predecessor = nPredecessor;
			if (edge.getV1().equals(nPredecessor))
			{
				portId = edge.getPortV1();
			}
			else
			{
				portId = edge.getPortV2();
			}
		}
		public PredecessorInfo (VertexInt nPredecessor)
		{
			predecessor = nPredecessor;
			portId = -1;
		}
	}
	private List<VertexInt> nodes;
	private List<Edge> edges;
	private Set<VertexInt> settledNodes;
	private Set<VertexInt> unSettledNodes;
	private Map<VertexInt, PredecessorInfo> predecessors;
	private Map<VertexInt, Integer> distance;
	  
	public void execute(VertexInt source) 
	{
		settledNodes = new HashSet<VertexInt>();
		unSettledNodes = new HashSet<VertexInt>();
		distance = new HashMap<VertexInt, Integer>();
		predecessors = new HashMap<VertexInt, PredecessorInfo>();
		distance.put(source, 0);
		unSettledNodes.add(source);
		while (unSettledNodes.size() > 0) 
		{
			VertexInt node = getMinimum(unSettledNodes);
		    settledNodes.add(node);
		    unSettledNodes.remove(node);
		    findMinimalDistances(node);
		}
	}
	
	private void findMinimalDistances(VertexInt node) 
	{
		List<VertexInt> adjacentNodes = getNeighbors(node);
		for (VertexInt target : adjacentNodes) 
		{
			Edge e = getDistance(node, target);
			if (getShortestDistance(target) > getShortestDistance(node) + e.getWeight()) 
		    {
				distance.put(target, getShortestDistance(node) +  e.getWeight());
		        
		        predecessors.put(target, new PredecessorInfo(node, e));
		        unSettledNodes.add(target);
		    }
		 }
	}
	
	private Edge getDistance(VertexInt node, VertexInt target)
	{
		for (Edge edge : edges) 
		{
			if (edge.isVertexIn(node) && edge.isVertexIn(target))
		    {
				//TODO: canviar per si doble enllaç
				return edge;
		    }
	    }
	    throw new RuntimeException("Should not happen");
	}
	
	private List<VertexInt> getNeighbors(VertexInt node)
	{
		List<VertexInt> neighbors = new ArrayList<VertexInt>();
		for (Edge edge : edges)
		{
			if (edge.isVertexIn(node) && !isSettled(edge.getOtherEndpoint(node)))
			{
				neighbors.add(edge.getOtherEndpoint(node));
			}
		}
		return neighbors;
	}
	
	  private VertexInt getMinimum(Set<VertexInt> vertexes) {
		  VertexInt minimum = null;
		    for (VertexInt vertex : vertexes) {
		      if (minimum == null) {
		        minimum = vertex;
		      } else {
		        if (getShortestDistance(vertex) < getShortestDistance(minimum)) {
		          minimum = vertex;
		        }
		      }
		    }
		    return minimum;
		  }
	
	  private boolean isSettled(VertexInt vertex) {
		    return settledNodes.contains(vertex);
		  }
	
	  private int getShortestDistance(VertexInt destination) {
		    Integer d = distance.get(destination);
		    if (d == null) {
		      return Integer.MAX_VALUE;
		    } else {
		      return d;
		    }
		  }
	
		  /*
	   * This method returns the path from the source to the selected target and
	   * NULL if no path exists
	   */
	  public LinkedList<PredecessorInfo> getPath(VertexInt target) 
	  {
		  LinkedList<PredecessorInfo> path = new LinkedList<PredecessorInfo>();
		  PredecessorInfo current = new PredecessorInfo(target);
		    // check if a path exists
	      if (predecessors.get(current.predecessor) == null) {
	        return null;
	      }
	      path.add(current);
	      while (predecessors.get(current.predecessor) != null) {
	        current = predecessors.get(current.predecessor);
	        path.add(current);
	      }
	      // Put it into the correct order
		  Collections.reverse(path);
		  return path;
	  }
	  
	  public PredecessorInfo getNextNode(VertexInt target, VertexInt source) 
	  {
		  PredecessorInfo step = new PredecessorInfo(target);
		  while (predecessors.get(step.predecessor) != null && predecessors.get(step.predecessor) != source) 
		  {
			  step = predecessors.get(step.predecessor);
		  }
		  if (step.predecessor.equals(target))
		  {
			  return null;
		  }
		  return step;
	  }
	  
	  
	  public PDUForwardingTableEntryList getPDUTForwardingTable(List<FlowStateObject> fsoList, VertexInt source)
	  {
		  Graph graph = new Graph(fsoList);
		  this.edges = new ArrayList<Edge>(graph.getEdges());
		  this.nodes = new ArrayList<VertexInt>();
		  this.nodes.addAll(graph.getVertices());
		  this.execute(source);
		  PDUForwardingTableEntryList pduftEntryList = new PDUForwardingTableEntryList();
		  
		  for (VertexInt v: this.nodes)
		  {
			  if(v.getAddress() != source.getAddress())
			  {
				  PredecessorInfo nextNode = getNextNode(v, source);
				  
				  
				  /*int portId = -1;
				  
				  for (FlowStateObject e : fsoList)
				  {
					  if (e.getNeighborAddress() == nextNode.getAddress() && e.getAddress() == source.getAddress())
					  {
						  // TODO: Weights for each FlowStateObject
						  portId = e.getPortid();
					  }
				  }*/
				  if (nextNode != null)
				  {
					  PDUForwardingTableEntry entry = new PDUForwardingTableEntry();
					  entry.setAddress(v.getAddress());
					  entry.addPortId(nextNode.portId);
					  entry.setQosId(1);
					  pduftEntryList.addLast(entry);
				  }
			  }
		  }
		  return pduftEntryList;
	  }
}
