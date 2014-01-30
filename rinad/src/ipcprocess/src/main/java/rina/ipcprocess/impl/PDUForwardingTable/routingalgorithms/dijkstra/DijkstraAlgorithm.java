package rina.ipcprocess.impl.PDUForwardingTable.routingalgorithms.dijkstra;

import rina.PDUForwardingTable.api.FlowStateObject;
import rina.ipcprocess.impl.PDUForwardingTable.routingalgorithms.RoutingAlgorithmInt;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.Set;

import eu.irati.librina.PDUForwardingTableEntry;
import eu.irati.librina.PDUForwardingTableEntryList;

public class DijkstraAlgorithm implements RoutingAlgorithmInt{
		
	private List<Vertex> nodes;
	private List<Edge> edges;
	private Set<Vertex> settledNodes;
	private Set<Vertex> unSettledNodes;
	private Map<Vertex, Vertex> predecessors;
	private Map<Vertex, Integer> distance;
	  
	public void execute(Vertex source) 
	{
		settledNodes = new HashSet<Vertex>();
		unSettledNodes = new HashSet<Vertex>();
		distance = new HashMap<Vertex, Integer>();
		predecessors = new HashMap<Vertex, Vertex>();
		distance.put(source, 0);
		unSettledNodes.add(source);
		while (unSettledNodes.size() > 0) 
		{
			Vertex node = getMinimum(unSettledNodes);
		    settledNodes.add(node);
		    unSettledNodes.remove(node);
		    findMinimalDistances(node);
		}
	}
	
	private void findMinimalDistances(Vertex node) 
	{
		List<Vertex> adjacentNodes = getNeighbors(node);
		for (Vertex target : adjacentNodes) 
		{
			Edge e = getDistance(node, target);
			if (getShortestDistance(target) > getShortestDistance(node) + e.getWeight()) 
		    {
				distance.put(target, getShortestDistance(node) +  e.getWeight());
		        
		        predecessors.put(target, node);
		        unSettledNodes.add(target);
		    }
		 }
	}
	
	private Edge getDistance(Vertex node, Vertex target)
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
	
	private List<Vertex> getNeighbors(Vertex node)
	{
		List<Vertex> neighbors = new ArrayList<Vertex>();
		for (Edge edge : edges)
		{
			if (edge.isVertexIn(node) && !isSettled(edge.getOtherEndpoint(node)))
			{
				neighbors.add(edge.getOtherEndpoint(node));
			}
		}
		return neighbors;
	}
	
	  private Vertex getMinimum(Set<Vertex> vertexes) {
		    Vertex minimum = null;
		    for (Vertex vertex : vertexes) {
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
	
	  private boolean isSettled(Vertex vertex) {
		    return settledNodes.contains(vertex);
		  }
	
	  private int getShortestDistance(Vertex destination) {
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
	  public LinkedList<Vertex> getPath(Vertex target) {
		    LinkedList<Vertex> path = new LinkedList<Vertex>();
		    Vertex step = target;
		    // check if a path exists
	    if (predecessors.get(step) == null) {
	      return null;
	    }
	    path.add(step);
	    while (predecessors.get(step) != null) {
	      step = predecessors.get(step);
	      path.add(step);
	    }
	    // Put it into the correct order
		    Collections.reverse(path);
		    return path;
	  }
	  
	  public Vertex getNextNode(Vertex target, Vertex source) 
	  {
		  Vertex step = target;
		  while (predecessors.get(step) != null && predecessors.get(step) != source) 
		  {
			  step = predecessors.get(step);
		  }
		  return step;
	  }
	  
	  
	  public PDUForwardingTableEntryList getPDUTForwardingTable(List<FlowStateObject> fsoList, Vertex source)
	  {
		  Graph graph = new Graph(fsoList);
		  this.edges = new ArrayList<Edge>(graph.getEdges());
		  this.nodes = new ArrayList<Vertex>(graph.getVertices());
		  this.execute(source);
		  PDUForwardingTableEntryList pduftEntryList = new PDUForwardingTableEntryList();
		  
		  for (Vertex v: this.nodes)
		  {
			  if(v.getAddress() != source.getAddress())
			  {
				  Vertex nextNode = getNextNode(v, source);
				  int portId = -1;
				  for (FlowStateObject e : fsoList)
				  {
					  if (e.getNeighborAddress() == nextNode.getAddress() && e.getAddress() == source.getAddress())
					  {
						  // TODO: Weights for each FlowStateObject
						  portId = e.getPortid();
					  }
				  }
				  if (portId != -1)
				  {
					  PDUForwardingTableEntry entry = new PDUForwardingTableEntry();
					  entry.setAddress(v.getAddress());
					  entry.addPortId(portId);
					  entry.setQosId(1);
					  pduftEntryList.addLast(entry);
				  }
			  }
		  }
		  return pduftEntryList;
	  }  

}
