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
import rina.cdap.api.CDAPSessionManager;
import rina.cdap.api.message.CDAPMessage;
import rina.cdap.api.message.ObjectValue;
import rina.encoding.api.Encoder;
import rina.events.api.Event;
import rina.events.api.EventListener;
import rina.ipcprocess.api.IPCProcess;
import rina.ipcprocess.impl.PDUForwardingTable.internalobjects.FlowStateInternalObject;
import rina.ipcprocess.impl.PDUForwardingTable.internalobjects.FlowStateInternalObjectGroup;
import rina.ipcprocess.impl.PDUForwardingTable.internalobjects.ObjectStateMapper;
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
import eu.irati.librina.ApplicationProcessNamingInformation;
import eu.irati.librina.FlowInformation;
import eu.irati.librina.Neighbor;
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
	protected final int NO_AVOID_PORT = -1;
	
	public final int WAIT_UNTIL_READ_CDAP = 5000;  //5 sec
	public final int WAIT_UNTIL_ERROR = 5000;  //5 sec
	public final int WAIT_UNTIL_PDUFT_COMPUTATION = 20000; // 100 ms
	public final int WAIT_UNTIL_FSODB_PROPAGATION = 60000; // 100 ms
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
	
	
	/**
	 * Constructor
	 */
	public PDUFTImpl (int maximumAge)
	{
		log.debug("PDUFT Created");
		db = new FlowStateDatabase();
		flowAllocatedList = new LinkedList<NMinusOneFlowAllocatedEvent>();
		this.maximumAge = maximumAge;
		
		/*	Time to compute PDUFT	*/
		/* TODO: Descomentar */
		pduFTComputationTimer = new Timer();
		pduFTComputationTimer.scheduleAtFixedRate(new ComputePDUFT(this), WAIT_UNTIL_PDUFT_COMPUTATION, WAIT_UNTIL_PDUFT_COMPUTATION);
		
		/*	Time to increment age	*/
		ageIncrementationTimer = new Timer();
		//ageIncrementationTimer.scheduleAtFixedRate(new UpdateAge(this), WAIT_UNTIL_AGE_INCREMENT, WAIT_UNTIL_AGE_INCREMENT);

		/* Timer to propagate modified FSO */
		fsodbPropagationTimer = new Timer();
		fsodbPropagationTimer.scheduleAtFixedRate(new PropagateFSODB(this), WAIT_UNTIL_FSODB_PROPAGATION, WAIT_UNTIL_FSODB_PROPAGATION);

		/* Timer to send CDAP*/
		sendCDAPTimers = new ArrayList<EnrollmentTimer>();
	}
	
	public void setIPCProcess(IPCProcess ipcProcess){
		this.ipcProcess = ipcProcess;
		//ipcManager = rina.getExtendedIPCManager();
		ribDaemon = ipcProcess.getRIBDaemon();
		encoder = ipcProcess.getEncoder();
		cdapSessionManager = ipcProcess.getCDAPSessionManager();
		//registrationManager = ipcProcess.getRegistrationManager();
		populateRIB();
		subscribeToEvents();
	}
	
	public void setAlgorithm(RoutingAlgorithmInt routingAlgorithm, VertexInt sourceVertex)
	{
		log.debug("Algorithm " + routingAlgorithm.getClass() + " setted");
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
			log.debug("Event n-1 flow deallocated treatment");
			NMinusOneFlowDeallocatedEvent flowDeEvent = (NMinusOneFlowDeallocatedEvent) event;
			flowDeallocated(flowDeEvent.getPortId());
			
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
		}else if (event.getId().equals(Event.N_MINUS_1_FLOW_ALLOCATED)){
			log.debug("Event n-1 flow allocated treatment");
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
			log.debug("Event neighbour added treatment");
			NeighborAddedEvent neighborAddedEvent = (NeighborAddedEvent) event;
			Neighbor neighbor= neighborAddedEvent.getNeighbor();
			ListIterator<NMinusOneFlowAllocatedEvent> iterate = flowAllocatedList.listIterator();
			boolean flowFound = false;
			while(!flowFound && iterate.hasNext())
			{
				NMinusOneFlowAllocatedEvent flowEvent = iterate.next();
				log.debug("flowEvent remote app name: " + flowEvent.getFlowInformation().getRemoteAppName().getProcessName());
				log.debug("neighbor name: " + neighbor.getName().getProcessName());
				if (flowEvent.getFlowInformation().getRemoteAppName().getProcessName().equals(neighbor.getName().getProcessName()) /*TODO:Add port*/)
				{
					try {
						flowAllocated(ipcProcess.getAddress(), flowEvent.getFlowInformation().getPortId(),
								ipcProcess.getAdressByname(flowEvent.getFlowInformation().getRemoteAppName()), 1);
						flowFound = true;
						iterate.remove();
						log.info("Flow allocated");
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
		log.debug("enrollmentToNeighbor function launched");
		ObjectStateMapper mapper = new ObjectStateMapper();
		ObjectValue objectValue = new ObjectValue();
		FlowStateObjectGroup fsg= mapper.FSOGMap(db.getFlowStateObjectGroup());
		
		// TODO: Que passa quan no tenim flow objects?
		if (fsg.getFlowStateObjectArray().size() > 0)
		{
			log.debug("Size of the group: " + fsg.getFlowStateObjectArray().size());
			log.debug("FSO address: " + fsg.getFlowStateObjectArray().get(0).getAddress() +
					" port: " +  fsg.getFlowStateObjectArray().get(0).getPortid() +
					" neighbor adress: " + fsg.getFlowStateObjectArray().get(0).getNeighborAddress() +
					" neighbourport" + fsg.getFlowStateObjectArray().get(0).getNeighborPortid());
			try 
			{	
				objectValue.setByteval(encoder.encode(fsg));
				CDAPMessage cdapMessage = cdapSessionManager.getWriteObjectRequestMessage(portId, null,
						null, FlowStateObjectGroup.FLOW_STATE_GROUP_RIB_OBJECT_CLASS, 0, objectValue, FlowStateObjectGroup.FLOW_STATE_GROUP_RIB_OBJECT_NAME, 0, false);
				ribDaemon.sendMessage(cdapMessage, portId , null);
				for (FlowStateInternalObject fsio: db.getFlowStateObjectGroup().getFlowStateObjectArray())
				{
					fsio.setAvoidPort(portId);
				}
			} catch (Exception e) {
				e.printStackTrace();
			}
			if (!newMember)
			{
				EnrollmentTimer timer = new EnrollmentTimer(portId);
				sendCDAPTimers.add(timer);
				timer.getTimer().schedule(new SendReadCDAP(portId, cdapSessionManager, ribDaemon, WAIT_UNTIL_ERROR), WAIT_UNTIL_READ_CDAP);
			}
		}
	}

	public void flowAllocated(long address, int portId, long neighborAddress, int neighborPortId)
	{
		log.debug("flowAllocated function launched");
		FlowStateInternalObject object = new FlowStateInternalObject(address, portId, neighborAddress, neighborPortId, true, 1, 0);
		db.addObjectToGroup(object, fsRIBGroup);
	}
	
	public boolean flowDeallocated(int portId)
	{
		log.debug("flowDeallocated function launched");
		FlowStateInternalObject object = db.getByPortId(portId);
		if (object == null)
		{
			return false;
		}
		
		object.setState(false);
		object.setAge(maximumAge);
		object.setSequenceNumber(object.getSequenceNumber()+1);
		object.setAvoidPort(NO_AVOID_PORT);
		object.setModified(true);
		
		db.setModified(true);
		return true;
	}
	
	public boolean propagateFSDB()
	{
		log.debug("propagateFSDB function launched");
		ArrayList<FlowStateInternalObject> modifiedFSOs;
		ObjectValue objectValue = new ObjectValue();
		ObjectStateMapper mapper = new  ObjectStateMapper();
		//TODO: Change for FlowStateObject no internal!
		ArrayList<FlowStateInternalObjectGroup> groupsToSend = new ArrayList<FlowStateInternalObjectGroup>();
		
		modifiedFSOs = db.getModifiedFSO();
		if (modifiedFSOs.size() == 0)
		{
			return true;
		}
		
		FlowInformation[] nminusFlowInfo = ipcProcess.getResourceAllocator().getNMinus1FlowManager().getAllNMinus1FlowsInformation();
		for(int i = 0; i < nminusFlowInfo.length; i++)
		{
			groupsToSend.add(new FlowStateInternalObjectGroup());
		}
	
		for(int i = 0; i < modifiedFSOs.size(); i++)
		{
			FlowStateInternalObject object = modifiedFSOs.get(i);
			log.debug("propagateFSDB(): Object to be propagated: " + object.getID());

			for(int j = 0; j < nminusFlowInfo.length; j++)
			{
				int portId = nminusFlowInfo[j].getPortId();
				if (object.getAvoidPort() != portId)
				{
					groupsToSend.get(j).addToSend(object);
				}
			}
			object.setModified(false);
			object.setAvoidPort(NO_AVOID_PORT);
		}
		
		for(int i = 0; i < nminusFlowInfo.length; i++)
		{
			try 
			{
				FlowStateObjectGroup fsg= mapper.FSOGMap(groupsToSend.get(i));
				objectValue.setByteval(encoder.encode(fsg));
				CDAPMessage cdapMessage = cdapSessionManager.getWriteObjectRequestMessage(
						nminusFlowInfo[i].getPortId(), null, null,
						FlowStateObjectGroup.FLOW_STATE_GROUP_RIB_OBJECT_CLASS, 0, objectValue, 
						FlowStateObjectGroup.FLOW_STATE_GROUP_RIB_OBJECT_NAME, 0, false);
				
				log.debug("Object Value sent: " + objectValue.getByteval());
				ribDaemon.sendMessage(cdapMessage, nminusFlowInfo[i].getPortId() , null);
			} catch (Exception e1) {
				e1.printStackTrace();
				return false;
			}
		}
		
		return true;
	}
	
	public void updateAge()
	{
		log.debug("updateAge function launched");
		db.incrementAge(maximumAge);
	}
	
	public void forwardingTableUpdate ()
	{
		log.debug("forwardingTableUpdate function launched");
		if (db.isModified())
		{
			ObjectStateMapper  mapper = new ObjectStateMapper();
			List<FlowStateObject> fsoList = mapper.FSOGMap(db.getFlowStateObjectGroup()).getFlowStateObjectArray();
			PDUForwardingTableEntryList entryList = routingAlgorithm.getPDUTForwardingTable(fsoList, (Vertex)sourceVertex);
			log.debug("PDUFT kernel entry list size: " + entryList.size());
			try {
				rina.getKernelIPCProcess().modifyPDUForwardingTableEntries(entryList, 2);
			} catch (PDUForwardingTableException e) {
				e.printStackTrace();
			}
			db.setModified(false);
			
			//Set A-Data PDU Forwarding Table in order to be able to relay management PDUs
			ribDaemon.setPDUForwardingTable(entryList);
		}
	}
	
	private void writeMessageRecieved(FlowStateObjectGroup objectsToModify, int srcPort)
	{
		ObjectStateMapper mapper = new ObjectStateMapper();
		
		FlowStateInternalObjectGroup intObjectsToModify = mapper.FSOGMap(objectsToModify);
		
		db.updateObjects(intObjectsToModify, srcPort, fsRIBGroup);
	}
	
	private void writeMessageRecieved(FlowStateObject objectToModify, int srcPort)
	{
		ArrayList<FlowStateObject> objectsToModifyList = new ArrayList<FlowStateObject>();
		objectsToModifyList.add(objectToModify);
		FlowStateObjectGroup objectsToModify = new FlowStateObjectGroup(objectsToModifyList);
		ObjectStateMapper mapper = new ObjectStateMapper();
		
		FlowStateInternalObjectGroup intObjectsToModify = mapper.FSOGMap(objectsToModify);
		
		db.updateObjects(intObjectsToModify, srcPort, fsRIBGroup);
	}

	@Override
	public boolean writeMessageRecieved(CDAPMessage objectsToModify, int srcPort) 
	{
		log.debug("writeMessageRecieved function launched");
		log.debug("Object of Class: " + objectsToModify.getObjClass());
		if (objectsToModify.getObjClass().equals(FlowStateObjectGroup.FLOW_STATE_GROUP_RIB_OBJECT_CLASS))
		{
			try {
				log.debug("Object Value recieved: " + objectsToModify.getObjValue().getByteval());
				FlowStateObjectGroup fsog =  (FlowStateObjectGroup)encoder.decode(objectsToModify.getObjValue().getByteval(), FlowStateObjectGroup.class);
				log.debug("Size of the group: " + fsog.getFlowStateObjectArray().size());
				log.debug("FSO address: " + fsog.getFlowStateObjectArray().get(0).getAddress() +
						" port: " +  fsog.getFlowStateObjectArray().get(0).getPortid() +
						" neighbor adress: " + fsog.getFlowStateObjectArray().get(0).getNeighborAddress() +
						" neighbourport" + fsog.getFlowStateObjectArray().get(0).getNeighborPortid());
				
				writeMessageRecieved(fsog, srcPort);
				int position = sendCDAPTimers.indexOf(new EnrollmentTimer(srcPort));
				if (position != -1)
				{
					EnrollmentTimer timer = sendCDAPTimers.get(position);
					timer.getTimer().cancel();
					sendCDAPTimers.remove(position);
				}
			} catch (Exception e) {
				e.printStackTrace();
				return false;
			}
		}
		else
		{
			try {
				FlowStateObject fso =  (FlowStateObject)encoder.decode(objectsToModify.getObjValue().getByteval(), FlowStateObject.class);
				writeMessageRecieved(fso, srcPort);
			} catch (Exception e) {
				e.printStackTrace();
				return false;
			}
		}
		return true;
	}
	
	public boolean readMessageRecieved(CDAPMessage objectsToModify, int srcPort)
	{
		log.debug("readMessageRecieved function launched");
		ObjectStateMapper mapper = new ObjectStateMapper();
		ObjectValue objectValue = new ObjectValue();
		FlowStateObjectGroup fsg= mapper.FSOGMap(db.getFlowStateObjectGroup());
		
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
