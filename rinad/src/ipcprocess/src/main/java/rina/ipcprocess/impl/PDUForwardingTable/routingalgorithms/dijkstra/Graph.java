package rina.ipcprocess.impl.PDUForwardingTable.routingalgorithms.dijkstra;

import java.util.ArrayList;
import java.util.List;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import rina.PDUForwardingTable.api.FlowStateObject;

public class Graph {
	  private static final Log log = LogFactory.getLog(Graph.class);
	  private List<Vertex> vertices;
	  private List<Edge> edges;
	  private List<FlowStateObject> flowStateObjects;

	  public Graph(List<FlowStateObject> flowStateO) {
		this.flowStateObjects = flowStateO;
		this.vertices = new ArrayList<Vertex>();
		this.edges = new ArrayList<Edge>();
	    initVertices();
	    try {
			initEdges();
		} catch (Exception e) {
			log.error("Vertex not found");
			e.printStackTrace();
		}
	  }

	  public List<Vertex> getVertices() {
	    return vertices;
	  }

	  public List<Edge> getEdges() {
	    return edges;
	  }
	  
	  private void initVertices()
	  {  
		  for (FlowStateObject e : this.flowStateObjects)
		  {
			  Vertex v1 = new Vertex(e.getAddress());
			  Vertex v2 = new Vertex(e.getNeighborAddress());
			  if (!vertices.contains(v1))
			  {
				  vertices.add(v1);
			  }
			  if (!vertices.contains(v2))
			  {
				  vertices.add(v2);
			  }
		  }
	  }
	  
	  private void initEdges() throws Exception
	  {
		    class VertexChecked{
		    	public Vertex v;
		    	public ArrayList<Vertex> connections;
		    	public int port;
		    	
		    	public VertexChecked(Vertex newV)
		    	{
		    		v = newV;
		    		connections = new ArrayList<Vertex>();
		    		port = -1;
		    	}
		    	
		    	@Override
		    	public boolean equals(Object obj) {
		    		if (this == obj)
		    		  return true;
		    		if (obj == null)
		    		  return false;
		    		if (getClass() != obj.getClass())
		    		  return false;
		    		VertexChecked other = (VertexChecked) obj;
		    		if (!v.equals(other.v))
		    		  return false;
		    		return true;
		    	}	
		    }
		    ArrayList<VertexChecked> verticesChecked = new ArrayList<VertexChecked>();
		    
		    for (Vertex v: vertices)
		    {
		    	verticesChecked.add(new VertexChecked(v));	
		    }
		    
//			int portV1 = -1;
//			int portV2 = -1;
			
			for (FlowStateObject f : flowStateObjects)
			{
				if (f.isState())
				{
					log.debug("Flow state object alive and processed: " + f.getID());
					int indexOrigin = verticesChecked.indexOf(new VertexChecked(new Vertex(f.getAddress())));
					int indexDest = verticesChecked.indexOf(new VertexChecked(new Vertex(f.getNeighborAddress())));
					
					if (indexOrigin != -1 && indexDest != -1)
					{
						VertexChecked origin = verticesChecked.get(indexOrigin);
						VertexChecked dest = verticesChecked.get(indexDest);
						
						if (origin.connections.contains(dest.v) && dest.connections.contains(origin.v))
						{
							edges.add(new Edge(origin.v, f.getPortid(), dest.v, dest.port, 1));
							origin.connections.remove(dest.v);
							dest.connections.remove(origin.v);
						}
						else
						{
							origin.port = f.getPortid();
							origin.connections.add(dest.v);
							dest.connections.add(origin.v);
						}
					}
					else
					{
						throw new Exception("Vertex not found"); //TODO: VertexNotFound exception
					}
				}
			}
		}
	  
	} 
