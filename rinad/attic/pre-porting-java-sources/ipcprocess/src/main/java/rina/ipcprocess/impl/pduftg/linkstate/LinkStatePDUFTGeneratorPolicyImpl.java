package rina.ipcprocess.impl.pduftg.linkstate;

import java.util.ArrayList;
import java.util.LinkedList;
import java.util.List;
import java.util.Timer;
import java.util.ListIterator;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import rina.cdap.api.CDAPException;
import rina.cdap.api.CDAPSessionManager;
import rina.cdap.api.message.CDAPMessage;
import rina.cdap.api.message.ObjectValue;
import rina.encoding.api.Encoder;
import rina.events.api.Event;
import rina.events.api.EventListener;
import rina.ipcprocess.api.IPCProcess;
import rina.ipcprocess.impl.events.NMinusOneFlowAllocatedEvent;
import rina.ipcprocess.impl.events.NMinusOneFlowDeallocatedEvent;
import rina.ipcprocess.impl.events.NeighborAddedEvent;
import rina.ipcprocess.impl.pduftg.linkstate.FlowStateDatabase;
import rina.ipcprocess.impl.pduftg.linkstate.PDUFTCDAPMessageHandler;
import rina.ipcprocess.impl.pduftg.linkstate.ribobjects.FlowStateRIBObjectGroup;
import rina.ipcprocess.impl.pduftg.linkstate.routingalgorithms.dijkstra.DijkstraAlgorithm;
import rina.ipcprocess.impl.pduftg.linkstate.routingalgorithms.dijkstra.Vertex;
import rina.ipcprocess.impl.pduftg.linkstate.timertasks.ComputePDUFT;
import rina.ipcprocess.impl.pduftg.linkstate.timertasks.PropagateFSODB;
import rina.ipcprocess.impl.pduftg.linkstate.timertasks.SendReadCDAP;
import rina.ipcprocess.impl.pduftg.linkstate.timertasks.UpdateAge;
import rina.pduftg.api.PDUFTGeneratorPolicy;
import rina.pduftg.api.linkstate.FlowStateObject;
import rina.pduftg.api.linkstate.FlowStateObjectGroup;
import rina.pduftg.api.linkstate.RoutingAlgorithmInt;
import rina.pduftg.api.linkstate.VertexInt;
import rina.ribdaemon.api.RIBDaemon;
import rina.ribdaemon.api.RIBDaemonException;
import eu.irati.librina.FlowInformation;
import eu.irati.librina.Neighbor;
import eu.irati.librina.PDUFTableGeneratorConfiguration;
import eu.irati.librina.PDUForwardingTableEntry;
import eu.irati.librina.PDUForwardingTableEntryList;
import eu.irati.librina.PDUForwardingTableException;
import eu.irati.librina.rina;

public class LinkStatePDUFTGeneratorPolicyImpl implements PDUFTGeneratorPolicy, EventListener {
	
	public class EnrollmentTimer
	{
		protected Timer timer = null;
		protected int portId = -1;
		
		public Timer getTimer() {
			return timer;
		}

		public void setTimer(Timer timer) {
			this.timer = timer;
		}

		public int getPortId() {
			return portId;
		}

		public EnrollmentTimer(int portId)
		{
			this.portId = portId;
			timer = new Timer();
		}
		@Override
		public boolean equals(Object obj) {
			if (this == obj)
			  return true;
			if (obj == null)
			  return false;
			if (getClass() != obj.getClass())
			  return false;
			EnrollmentTimer other = (EnrollmentTimer) obj;
			if (portId != other.portId)
			  return false;
			return true;
		}
	}
	
	private static final Log log = LogFactory.getLog(LinkStatePDUFTGeneratorPolicyImpl.class);
	
	protected final int MAXIMUM_BUFFER_SIZE = 4096;
	
	
	protected Timer pduFTComputationTimer = null;
	protected Timer ageIncrementationTimer = null;
	protected Timer fsodbPropagationTimer = null;
	protected List<EnrollmentTimer> sendCDAPTimers = null;
	
	
	protected VertexInt sourceVertex;
	protected int maximumAge = 2147483647; 

	protected RoutingAlgorithmInt routingAlgorithm;
	/**
	 * The IPC process
	 */
	protected IPCProcess ipcProcess = null;
	
	/**
	 * The RIB Daemon
	 */
	protected RIBDaemon ribDaemon = null;
	
	protected Encoder encoder = null;
	
	protected CDAPSessionManager cdapSessionManager = null;
	
	/**
	 * The Flow State Database
	 */
	protected FlowStateDatabase db = null;
	protected FlowStateRIBObjectGroup fsRIBGroup = null;
	
	/**
	 * Flow allocated queue
	 */
	protected List<NMinusOneFlowAllocatedEvent> flowAllocatedList = null;
	
	/**
	 * PDUFT CDAP message handler
	 */
	protected PDUFTCDAPMessageHandler pduftCDAPmessageHandler = null;
	
	/**
	 * Configurations of the PDUFT generator
	 */
	protected PDUFTableGeneratorConfiguration pduftGeneratorConfiguration = null; 
	
	protected boolean test = false;
	public void setTest(boolean test)
	{
		this.test = test;
	}
	public FlowStateDatabase getDB()
	{
		return this.db;
	}
	
	/**
	 * Constructor
	 */
	public LinkStatePDUFTGeneratorPolicyImpl ()
	{
		log.info("PDUFTGenerator Created");
		db = new FlowStateDatabase();
		flowAllocatedList = new LinkedList<NMinusOneFlowAllocatedEvent>();
		sendCDAPTimers = new ArrayList<EnrollmentTimer>();
		pduftCDAPmessageHandler = new PDUFTCDAPMessageHandler(this);
	}
	
	public void setIPCProcess(IPCProcess ipcProcess){
		this.ipcProcess = ipcProcess;
		ribDaemon = ipcProcess.getRIBDaemon();
		encoder = ipcProcess.getEncoder();
		cdapSessionManager = ipcProcess.getCDAPSessionManager();
		populateRIB();
		subscribeToEvents();
	}
	
	public void setDIFConfiguration(PDUFTableGeneratorConfiguration pduftgConfig)
	{
		if (pduftgConfig == null) {
			log.warn("PDU Forwarding Table Configuration is null, using a default one");
			pduftgConfig = new PDUFTableGeneratorConfiguration();
			log.warn("Defulat config: " +
					pduftgConfig.getLinkStateRoutingConfiguration().toString());
		}
		
		pduftGeneratorConfiguration = pduftgConfig;
		String routingAlgorithmName = pduftGeneratorConfiguration.getLinkStateRoutingConfiguration().getRoutingAlgorithm();
		if (!routingAlgorithmName.equals("Dijkstra")) {
			log.warn("Routing algorithm: "+routingAlgorithmName+" currently not supported, using Dijkstra instead");
		}
		
		RoutingAlgorithmInt routingAlgorithm = new DijkstraAlgorithm();
		VertexInt sourceVertex = new Vertex(ipcProcess.getAddress());
		log.info("Algorithm " + routingAlgorithm.getClass() + " setted");
		this.routingAlgorithm = routingAlgorithm;
		this.sourceVertex = sourceVertex;
		if(!test)
		{
			this.maximumAge = pduftGeneratorConfiguration.getLinkStateRoutingConfiguration().getObjectMaximumAge();

			/*	Time to compute PDUFT	*/	
			pduFTComputationTimer = new Timer();
			pduFTComputationTimer.scheduleAtFixedRate(new ComputePDUFT(this), 
					pduftGeneratorConfiguration.getLinkStateRoutingConfiguration().getWaitUntilPDUFTComputation(), 
					pduftGeneratorConfiguration.getLinkStateRoutingConfiguration().getWaitUntilPDUFTComputation());
			
			/*	Time to increment age	*/
			ageIncrementationTimer = new Timer();
			ageIncrementationTimer.scheduleAtFixedRate(new UpdateAge(this), 
					pduftGeneratorConfiguration.getLinkStateRoutingConfiguration().getWaitUntilAgeIncrement(), 
					pduftGeneratorConfiguration.getLinkStateRoutingConfiguration().getWaitUntilAgeIncrement());
	
			/* Timer to propagate modified FSO */
			fsodbPropagationTimer = new Timer();
			fsodbPropagationTimer.scheduleAtFixedRate(new PropagateFSODB(this), 
					pduftGeneratorConfiguration.getLinkStateRoutingConfiguration().getWaitUntilFSODBPropagation(), 
					pduftGeneratorConfiguration.getLinkStateRoutingConfiguration().getWaitUntilFSODBPropagation());
		}
	}
	
	public void setMaximumAge(int maximumAge)
	{
		this.maximumAge = maximumAge;
	}
	
	private void populateRIB(){
		try{
			fsRIBGroup = new FlowStateRIBObjectGroup(this, ipcProcess);
			ribDaemon.addRIBObject(fsRIBGroup);
		}catch(Exception ex){
			log.error("Could not subscribe to RIB Daemon:" +ex.getMessage());
		}
	}
	
	private void subscribeToEvents(){
		this.ribDaemon.subscribeToEvent(Event.N_MINUS_1_FLOW_DEALLOCATED, this);
		this.ribDaemon.subscribeToEvent(Event.N_MINUS_1_FLOW_ALLOCATED, this);
		this.ribDaemon.subscribeToEvent(Event.NEIGHBOR_ADDED, this);
	}
	
	/**
	 * Called when the events we're subscribed to happen
	 */
	public void eventHappened(Event event) {
		if (event.getId().equals(Event.N_MINUS_1_FLOW_DEALLOCATED)){
			log.debug("Event n-1 flow deallocated launched");
			NMinusOneFlowDeallocatedEvent flowDeEvent = (NMinusOneFlowDeallocatedEvent) event;
			
			ListIterator<NMinusOneFlowAllocatedEvent> iterate = flowAllocatedList.listIterator();
			boolean flowFound = false;
			
			while(!flowFound && iterate.hasNext())
			{
				NMinusOneFlowAllocatedEvent flowEvent = iterate.next();
				if (flowEvent.getFlowInformation().getPortId() == flowDeEvent.getPortId())
				{
					flowFound = true;
					iterate.remove();
				}
			}
			if (!flowFound)
			{
				flowDeallocated(flowDeEvent.getPortId());
			}
		}else if (event.getId().equals(Event.N_MINUS_1_FLOW_ALLOCATED)){
			log.debug("Event n-1 flow allocated launched");
			NMinusOneFlowAllocatedEvent flowEvent = (NMinusOneFlowAllocatedEvent) event;
			/*	Check if enrolled before flow allocation */
			try 
			{
				flowAllocated(ipcProcess.getAddress(), flowEvent.getFlowInformation().getPortId(),
						ipcProcess.getAdressByname(flowEvent.getFlowInformation().getRemoteAppName()), 1);
			} catch (Exception e) {
				log.debug("flow allocation waiting for enrollment");
				flowAllocatedList.add(flowEvent);
			}
		}else if (event.getId().equals(Event.NEIGHBOR_ADDED))
		{
			log.debug("Event neighbour added launched");
			NeighborAddedEvent neighborAddedEvent = (NeighborAddedEvent) event;
			Neighbor neighbor= neighborAddedEvent.getNeighbor();
			ListIterator<NMinusOneFlowAllocatedEvent> iterate = flowAllocatedList.listIterator();
			boolean flowFound = false;
			while(!flowFound && iterate.hasNext())
			{
				NMinusOneFlowAllocatedEvent flowEvent = iterate.next();
				if (flowEvent.getFlowInformation().getRemoteAppName().getProcessName().equals(neighbor.getName().getProcessName()) /*TODO:Add port*/)
				{
					log.info("There was an allocation flow event waiting for enrollment, launching it");
					try {
						flowAllocated(ipcProcess.getAddress(), flowEvent.getFlowInformation().getPortId(),
								ipcProcess.getAdressByname(flowEvent.getFlowInformation().getRemoteAppName()), 1);
						flowFound = true;
						iterate.remove();
					} catch (Exception e) {
						log.error("Could not allocate the flow, no neighbour found");
						e.printStackTrace();
					}
				}
			}
			// TODO: Remove newmember thing
			enrollmentToNeighbor(neighbor.getAddress(), true , neighbor.getUnderlyingPortId());
		}
	}
	
	public void enrollmentToNeighbor(long address, boolean newMember, int portId)
	{
		ObjectValue objectValue = new ObjectValue();
		if (!db.isEmpty())
		{
			try 
			{							
				objectValue.setByteval(db.encode(encoder));
				CDAPMessage cdapMessage = cdapSessionManager.getWriteObjectRequestMessage(portId, null,
						null, FlowStateObjectGroup.FLOW_STATE_GROUP_RIB_OBJECT_CLASS, 0, objectValue, 
						FlowStateObjectGroup.FLOW_STATE_GROUP_RIB_OBJECT_NAME, 0, false);
				ribDaemon.sendMessage(cdapMessage, portId , null);
				db.setAvoidPort(portId);
			} catch (Exception e) {
				e.printStackTrace();
			}
			if (!newMember)
			{
				log.debug("Launch timer to check write back");
				EnrollmentTimer timer = new EnrollmentTimer(portId);
				sendCDAPTimers.add(timer);
				timer.getTimer().schedule(new SendReadCDAP(portId, cdapSessionManager, ribDaemon, 
						pduftGeneratorConfiguration.getLinkStateRoutingConfiguration().getWaitUntilError(), pduftCDAPmessageHandler), 
						pduftGeneratorConfiguration.getLinkStateRoutingConfiguration().getWaitUntilReadCDAP());
			}
		}
	}

	public void flowAllocated(long address, int portId, long neighborAddress, int neighborPortId)
	{
		log.info("flowAllocated function launched PortID:" + portId);
		db.addObjectToGroup(address, portId, neighborAddress, neighborPortId, fsRIBGroup);
	}
	
	public boolean flowDeallocated(int portId)
	{
		log.info("flowDeallocated function launched");
		return db.deprecateObject(portId, maximumAge);
	}
	
	public boolean propagateFSDB()
	{
		ObjectValue objectValue = new ObjectValue();
		
		FlowInformation[] nminusFlowInfo = ipcProcess.getResourceAllocator().getNMinus1FlowManager().getAllNMinus1FlowsInformation();
		
		ArrayList<FlowStateObjectGroup> groupsToSend = db.prepareForPropagation(nminusFlowInfo);
		
		if (!groupsToSend.isEmpty())
		{
			for(int i = 0; i < nminusFlowInfo.length; i++)
			{
				log.debug("Flow info num:" + i + " corresponding to neighbor: " + nminusFlowInfo[i].getRemoteAppName() 
						+ " and port: " + nminusFlowInfo[i].getPortId());
				try 
				{
					FlowStateObjectGroup fsg= groupsToSend.get(i);
					if (fsg.getFlowStateObjectArray().size() > 0)
					{
						objectValue.setByteval(encoder.encode(fsg));
						CDAPMessage cdapMessage = cdapSessionManager.getWriteObjectRequestMessage(
								nminusFlowInfo[i].getPortId(), null, null,
								FlowStateObjectGroup.FLOW_STATE_GROUP_RIB_OBJECT_CLASS, 0, objectValue, 
								FlowStateObjectGroup.FLOW_STATE_GROUP_RIB_OBJECT_NAME, 0, false);
						ribDaemon.sendMessage(cdapMessage, nminusFlowInfo[i].getPortId() , null);
					}
				}
				catch (CDAPException e1)
				{
					log.error("Error creating the CDAP message to propagate FSDB");
					e1.printStackTrace();
				}
				catch (RIBDaemonException e2)
				{
					log.error("Error sending the CDAP message to propagate the FSDB");
					e2.printStackTrace();
				}
				catch (Exception e3) {
					e3.printStackTrace();
					log.error("Error encoding the message");
				}
			}
			return true;
		}
		return false;
	}
	
	public void updateAge()
	{
		try {
			db.incrementAge(maximumAge, fsRIBGroup);
		} catch (RIBDaemonException e) {
			log.error("Error removing an object from the RIB");
			e.printStackTrace();
		}
	}
	
	public void forwardingTableUpdate ()
	{
		if (db.isModified())
		{
			db.setModified(false);
			List<FlowStateObject> fsoList = db.getFlowStateObjectGroup().getFlowStateObjectArray();
			PDUForwardingTableEntryList entryList = routingAlgorithm.getPDUTForwardingTable(fsoList, (Vertex)sourceVertex);
			try {
				rina.getKernelIPCProcess().modifyPDUForwardingTableEntries(entryList, 2);
				for (PDUForwardingTableEntry e : entryList)
				{
					log.debug("Entry set in kernel. Address: " + e.getAddress() + " Port: " + e.getPortIds().getFirst());
				}
			} catch (PDUForwardingTableException e) {
				log.error("Error setting the PDU Forwarding table in the kernel");
				e.printStackTrace();
			}
		}
	}
	
	public boolean writeMessageRecieved(CDAPMessage objectsToModify, int srcPort) 
	{
		if (objectsToModify.getObjClass().equals(FlowStateObjectGroup.FLOW_STATE_GROUP_RIB_OBJECT_CLASS))
		{
			try {
				FlowStateObjectGroup fsog =  (FlowStateObjectGroup)encoder.decode(objectsToModify.getObjValue().getByteval(), FlowStateObjectGroup.class);			
				db.updateObjects(fsog, srcPort, fsRIBGroup, ipcProcess.getAddress());
				int position = sendCDAPTimers.indexOf(new EnrollmentTimer(srcPort));
				if (position != -1)
				{
					log.debug("Cancel timer");
					EnrollmentTimer timer = sendCDAPTimers.get(position);
					timer.getTimer().cancel();
					sendCDAPTimers.remove(position);
				}
				return true;
			} catch (Exception e) {
				e.printStackTrace();
			}
		}
		return false;
	}
	
	public boolean readMessageRecieved(CDAPMessage objectsToModify, int srcPort)
	{
		ObjectValue objectValue = new ObjectValue();
		FlowStateObjectGroup fsg= db.getFlowStateObjectGroup();
		
		if (objectsToModify.getObjClass().equals(FlowStateObjectGroup.FLOW_STATE_GROUP_RIB_OBJECT_CLASS))
		{
			CDAPMessage cdapMessage;
			try {
				objectValue.setByteval(encoder.encode(fsg));
				cdapMessage = cdapSessionManager.getReadObjectResponseMessage(srcPort, null, 
						FlowStateObjectGroup.FLOW_STATE_GROUP_RIB_OBJECT_CLASS, 0, FlowStateObjectGroup.FLOW_STATE_GROUP_RIB_OBJECT_NAME,
						objectValue, 0, null, 0);
				ribDaemon.sendMessage(cdapMessage, srcPort , null);
			} catch (CDAPException e) {
				log.error("Error creating the CDAP message");
				e.printStackTrace();
			}
			catch (RIBDaemonException e) {
				log.error("Error sending the CDAP message");
				e.printStackTrace();
			}
			catch (Exception e) {
				log.error("Error codifying the flow state object group");
				e.printStackTrace();
			}
		}
		return true;
	}
}
