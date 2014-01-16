package rina.ipcprocess.impl.PDUForwardingTable.routingalgorithm;

import rina.PDUForwardingTable.api.FlowStateObjectGroup;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.Set;

public class DijkstraAlgorithm implements RoutingAlgorithmInt{
	
	  private final List<FlowStateObjectGroup> nodes;
	  private final List<FlowStateObjectGroup> edges;
	  private Set<FlowStateObjectGroup> settledNodes;
	  private Set<FlowStateObjectGroup> unSettledNodes;
	  private Map<FlowStateObjectGroup, FlowStateObjectGroup> predecessors;
	  private Map<FlowStateObjectGroup, FlowStateObjectGroup> distance;
	
	public FlowStateObjectGroup computePath(FlowStateObjectGroup fsos)
	{
		
		
	}
	

}
