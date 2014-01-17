package rina.ipcprocess.impl.PDUForwardingTable.routingalgorithms.dijkstra;

import java.util.List;
import java.util.TreeSet;

import rina.PDUForwardingTable.api.FlowStateObject;

public class Graph {
	  private List<Vertex> vertices;
	  private List<Edge> edges;
	  private List<FlowStateObject> flowStateObjects;

	  public Graph(List<FlowStateObject> flowStateO) {
		
		this.flowStateObjects = flowStateO;
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
		  
		  TreeSet<Vertex> notRepeatedOrigin = new TreeSet<Vertex>();
		  TreeSet<Vertex> notRepeatedDest = new TreeSet<Vertex>();
		  
		  for (FlowStateObject e : this.flowStateObjects)
		  {
			  notRepeatedOrigin.add(new Vertex(e.getAddress(), e.getPortid()));
			  notRepeatedDest.add(new Vertex(e.getNeighborAddress(), e.getNeighborPortid()));
		  }
		  notRepeatedOrigin.retainAll(notRepeatedDest);
		  
		  vertices.addAll(notRepeatedOrigin);
	  }
	  
	  private void initEdges()
	  {
			boolean originChecked;
			boolean destChecked;
			Vertex origin = null;
			Vertex dest = null;
			int i;
			
			for (FlowStateObject f : flowStateObjects)
			{
				i = 0;
				originChecked = false;
				destChecked = false;
				while(!originChecked && !destChecked && i < vertices.size())
				{
					Vertex v = vertices.get(i);
					if (v.getAddress() == f.getAddress() && v.getPortId() == f.getPortid())
					{
						origin = v;
						originChecked = true;
					}
					if (v.getAddress() == f.getNeighborAddress() && v.getPortId() == f.getNeighborPortid())
					{
						dest = v;
						destChecked = true;
					}
					i++;
				}
				if (!originChecked || !destChecked)
				{
					flowStateObjects.remove(f);
				}
				else
				{
					this.edges.add(new Edge(origin, dest, 1));			
				}
			}
		}
	  
	} 
