package rina.ipcprocess.impl.PDUForwardingTable.routingalgorithms.dijkstra;

import java.util.ArrayList;
import java.util.List;
import java.util.HashSet;

import rina.PDUForwardingTable.api.FlowStateObject;

public class Graph {
	  private List<Vertex> vertices;
	  private List<Edge> edges;
	  private List<FlowStateObject> flowStateObjects;

	  public Graph(List<FlowStateObject> flowStateO) {
		this.flowStateObjects = flowStateO;
		this.vertices = new ArrayList<Vertex>();
		this.edges = new ArrayList<Edge>();
	    initVertices();
	    initEdges();
	  }

	  public List<Vertex> getVertices() {
	    return vertices;
	  }

	  public List<Edge> getEdges() {
	    return edges;
	  }
	  
	  private void initVertices()
	  {
		  
		  HashSet<Vertex> notRepeatedOrigin = new HashSet<Vertex>();
		  HashSet<Vertex> notRepeatedDest = new HashSet<Vertex>();
		  
		  for (FlowStateObject e : this.flowStateObjects)
		  {
			  Vertex v1 = new Vertex(e.getAddress());
			  Vertex v2 = new Vertex(e.getNeighborAddress());
			  notRepeatedOrigin.add(v1);
			  notRepeatedDest.add(v2);
		  }
		  notRepeatedOrigin.retainAll(notRepeatedDest);
		
		  vertices.addAll(notRepeatedOrigin);
	  }
	  
	  private void initEdges()
	  {
			boolean v1Checked;
			boolean v2Checked;
			Vertex v1 = null;
			Vertex v2 = null;
//			int portV1 = -1;
//			int portV2 = -1;
			int i;
			
			for (FlowStateObject f : flowStateObjects)
			{
				i = 0;
				v1Checked = false;
				v2Checked = false;
				while((!v1Checked || !v2Checked) && i < vertices.size())
				{
					Vertex v = vertices.get(i);
					if (v.getAddress() == f.getAddress())
					{
						v1 = v;
						v1Checked = true;
//						portV1 = f.getPortid();
					}
					if (v.getAddress() == f.getNeighborAddress())
					{
						v2 = v;
						v2Checked = true;
//						portV2 = f.getNeighborPortid();
					}
					i++;
				}
				if (v1Checked && v2Checked && !this.edges.contains(new Edge(v2, v1, 1)))
				{
					this.edges.add(new Edge(v1, v2, 1));			
				}
			}
		}
	  
	} 
