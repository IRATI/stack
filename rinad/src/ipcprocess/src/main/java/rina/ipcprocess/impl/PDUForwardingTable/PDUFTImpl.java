package rina.ipcprocess.impl.PDUForwardingTable;

import java.util.ArrayList;
import java.util.LinkedList;
import java.util.List;
import java.util.Timer;
import java.util.ListIterator;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import rina.PDUForwardingTable.api.FlowStateObject;
import rina.PDUForwardingTable.api.FlowStateObjectGroup;
import rina.PDUForwardingTable.api.PDUFTable;
import rina.PDUForwardingTable.api.RoutingAlgorithmInt;
import rina.PDUForwardingTable.api.VertexInt;
import rina.cdap.api.CDAPException;
import rina.cdap.api.CDAPSessionManager;
import rina.cdap.api.message.CDAPMessage;
import rina.cdap.api.message.ObjectValue;
import rina.encoding.api.Encoder;
import rina.events.api.Event;
import rina.events.api.EventListener;
import rina.ipcprocess.api.IPCProcess;
import rina.ipcprocess.impl.PDUForwardingTable.ribobjects.FlowStateRIBObjectGroup;
import rina.ipcprocess.impl.PDUForwardingTable.routingalgorithms.dijkstra.Vertex;
import rina.ipcprocess.impl.PDUForwardingTable.timertasks.ComputePDUFT;
import rina.ipcprocess.impl.PDUForwardingTable.timertasks.PropagateFSODB;
import rina.ipcprocess.impl.PDUForwardingTable.timertasks.SendReadCDAP;
import rina.ipcprocess.impl.PDUForwardingTable.timertasks.UpdateAge;
import rina.ipcprocess.impl.events.NMinusOneFlowAllocatedEvent;
import rina.ipcprocess.impl.events.NMinusOneFlowDeallocatedEvent;
import rina.ipcprocess.impl.events.NeighborAddedEvent;
import rina.ribdaemon.api.RIBDaemon;
import rina.ribdaemon.api.RIBDaemonException;
import eu.irati.librina.ApplicationProcessNamingInformation;
import eu.irati.librina.FlowInformation;
import eu.irati.librina.Neighbor;
import eu.irati.librina.PDUForwardingTableEntry;
import eu.irati.librina.PDUForwardingTableEntryList;
import eu.irati.librina.PDUForwardingTableException;
import eu.irati.librina.rina;

public class PDUFTImpl implements PDUFTable, EventListener {
	
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
	
	private static final Log log = LogFactory.getLog(PDUFTImpl.class);
	
	protected final int MAXIMUM_BUFFER_SIZE = 4096;
	
	public final int WAIT_UNTIL_READ_CDAP = 5000;  //5 sec
	public final int WAIT_UNTIL_ERROR = 5000;  //5 sec
	public final int WAIT_UNTIL_PDUFT_COMPUTATION = 100; // 100 ms
	public final int WAIT_UNTIL_FSODB_PROPAGATION = 100; // 100 ms
	public final int WAIT_UNTIL_AGE_INCREMENT = 3000; //3 sec
	
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
	public PDUFTImpl (int maximumAge)
	{
		log.info("PDUFT Created");
		db = new FlowStateDatabase();
		flowAllocatedList = new LinkedList<NMinusOneFlowAllocatedEvent>();
		this.maximumAge = maximumAge;
	}
	
	public void setIPCProcess(IPCProcess ipcProcess){
		this.ipcProcess = ipcProcess;
		ribDaemon = ipcProcess.getRIBDaemon();
		encoder = ipcProcess.getEncoder();
		cdapSessionManager = ipcProcess.getCDAPSessionManager();
		populateRIB();
		subscribeToEvents();
		
		/*	Time to compute PDUFT	*/
		/* TODO: Descomentar */
		pduFTComputationTimer = new Timer();
		pduFTComputationTimer.scheduleAtFixedRate(new ComputePDUFT(this), WAIT_UNTIL_PDUFT_COMPUTATION, WAIT_UNTIL_PDUFT_COMPUTATION);
		
		/*	Time to increment age	*/
		ageIncrementationTimer = new Timer();
		ageIncrementationTimer.scheduleAtFixedRate(new UpdateAge(this), WAIT_UNTIL_AGE_INCREMENT, WAIT_UNTIL_AGE_INCREMENT);

		/* Timer to propagate modified FSO */
		fsodbPropagationTimer = new Timer();
		fsodbPropagationTimer.scheduleAtFixedRate(new PropagateFSODB(this), WAIT_UNTIL_FSODB_PROPAGATION, WAIT_UNTIL_FSODB_PROPAGATION);

		/* Timer to send CDAP*/
		sendCDAPTimers = new ArrayList<EnrollmentTimer>();
	}
	
	public void setAlgorithm(RoutingAlgorithmInt routingAlgorithm, VertexInt sourceVertex)
	{
		log.info("Algorithm " + routingAlgorithm.getClass() + " setted");
		this.routingAlgorithm = routingAlgorithm;
		this.sourceVertex = sourceVertex;
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
			enrollmentToNeighbor(neighbor.getName(), neighbor.getAddress(), true , neighbor.getUnderlyingPortId());
		}
	}
	
	public void enrollmentToNeighbor(ApplicationProcessNamingInformation name, long address, boolean newMember, int portId)
	{
		ObjectValue objectValue = new ObjectValue();
		
		if (!db.isEmpty())
		{
			try 
			{	
				objectValue.setByteval(db.encode(encoder));
				CDAPMessage cdapMessage = cdapSessionManager.getWriteObjectRequestMessage(portId, null,
						null, FlowStateObjectGroup.FLOW_STATE_GROUP_RIB_OBJECT_CLASS, 0, objectValue, FlowStateObjectGroup.FLOW_STATE_GROUP_RIB_OBJECT_NAME, 0, false);
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
				timer.getTimer().schedule(new SendReadCDAP(portId, cdapSessionManager, ribDaemon, WAIT_UNTIL_ERROR), WAIT_UNTIL_READ_CDAP);
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
		//log.debug("propagateFSDB function launched");

		ObjectValue objectValue = new ObjectValue();
		
		FlowInformation[] nminusFlowInfo = ipcProcess.getResourceAllocator().getNMinus1FlowManager().getAllNMinus1FlowsInformation();
		log.debug("nminusFloxInfo length: " + nminusFlowInfo.length);
		
		ArrayList<FlowStateObjectGroup> groupsToSend = db.prepareForPropagation(nminusFlowInfo);
		log.debug("groupsToSend size: " + groupsToSend.size());
		log.debug("group size: " + db.getFlowStateObjectGroup().getFlowStateObjectArray().size());
		
		if (groupsToSend.size() > 0)
		{
			log.debug("hola1");
			for(int i = 0; i < nminusFlowInfo.length; i++)
			{
				log.debug("hola2");
				try 
				{
					FlowStateObjectGroup fsg= groupsToSend.get(i);
					if (fsg.getFlowStateObjectArray().size() > 0)
					{
						log.debug("Group sent: " + fsg.getFlowStateObjectArray());
						objectValue.setByteval(encoder.encode(fsg));
						CDAPMessage cdapMessage = cdapSessionManager.getWriteObjectRequestMessage(
								nminusFlowInfo[i].getPortId(), null, null,
								FlowStateObjectGroup.FLOW_STATE_GROUP_RIB_OBJECT_CLASS, 0, objectValue, 
								FlowStateObjectGroup.FLOW_STATE_GROUP_RIB_OBJECT_NAME, 0, false);
						ribDaemon.sendMessage(cdapMessage, nminusFlowInfo[i].getPortId() , null);
					}
					return true;
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
		}
		return false;
	}
	
	public void updateAge()
	{
		//log.debug("updateAge function launched");
		try {
			db.incrementAge(maximumAge, fsRIBGroup);
		} catch (RIBDaemonException e) {
			log.error("Error removing an object from the RIB");
			e.printStackTrace();
		}
	}
	
	public void forwardingTableUpdate ()
	{
		//log.debug("forwardingTableUpdate function launched");
		if (db.isModified())
		{
			db.setModified(false);
			log.debug("FSDB is modified, computing new paths");
			List<FlowStateObject> fsoList = db.getFlowStateObjectGroup().getFlowStateObjectArray();
			PDUForwardingTableEntryList entryList = routingAlgorithm.getPDUTForwardingTable(fsoList, (Vertex)sourceVertex);
			try {
				rina.getKernelIPCProcess().modifyPDUForwardingTableEntries(entryList, 2);
				ribDaemon.setPDUForwardingTable(entryList);
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
	
	@Override
	public boolean writeMessageRecieved(CDAPMessage objectsToModify, int srcPort) 
	{
		log.info("writeMessageRecieved function launched");
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
		log.info("readMessageRecieved function launched");
		ObjectValue objectValue = new ObjectValue();
		FlowStateObjectGroup fsg= db.getFlowStateObjectGroup();
		
		if (objectsToModify.getObjClass().equals(FlowStateObjectGroup.FLOW_STATE_GROUP_RIB_OBJECT_CLASS))
		{
			int position = sendCDAPTimers.indexOf(new EnrollmentTimer(srcPort));
			if (position != -1)
			{
				try 
				{
					
					objectValue.setByteval(encoder.encode(fsg));
					CDAPMessage cdapMessage = cdapSessionManager.getReadObjectResponseMessage(srcPort, null, 
							FlowStateObjectGroup.FLOW_STATE_GROUP_RIB_OBJECT_CLASS, 0, FlowStateObjectGroup.FLOW_STATE_GROUP_RIB_OBJECT_NAME,
							objectValue, 0, null, 0);
					ribDaemon.sendMessage(cdapMessage, srcPort , null);
					
					// TODO: Implementar anulació segon timer
					/*if (position != -1)
					{
						SendReadCDAP timer = (SendReadCDAP)sendCDAPTimers.get(position).getTimer();
						timer.getTimer().
						sendCDAPTimers.remove(position);
					}	*/		
				}
				catch (Exception e) 
				{
					e.printStackTrace();
					return false;
				}
			}
		}
		return true;
	}
}
