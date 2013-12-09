package rina.ipcprocess.impl.flowallocator;

import java.util.Timer;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import eu.irati.librina.AllocateFlowResponseEvent;
import eu.irati.librina.CreateConnectionResponseEvent;
import eu.irati.librina.CreateConnectionResultEvent;
import eu.irati.librina.ExtendedIPCManagerSingleton;
import eu.irati.librina.FlowDeallocateRequestEvent;
import eu.irati.librina.FlowRequestEvent;
import eu.irati.librina.IPCException;
import eu.irati.librina.KernelIPCProcessSingleton;
import eu.irati.librina.UpdateConnectionResponseEvent;
import eu.irati.librina.rina;

import rina.cdap.api.CDAPMessageHandler;
import rina.cdap.api.CDAPSessionDescriptor;
import rina.cdap.api.CDAPSessionManager;
import rina.cdap.api.message.CDAPMessage;
import rina.cdap.api.message.ObjectValue;
import rina.encoding.api.Encoder;
import rina.flowallocator.api.FlowAllocator;
import rina.flowallocator.api.FlowAllocatorInstance;
import rina.flowallocator.api.Flow;
import rina.flowallocator.api.Flow.State;
import rina.ipcprocess.impl.IPCProcess;
import rina.ipcprocess.impl.flowallocator.policies.NewFlowRequestPolicy;
import rina.ipcprocess.impl.flowallocator.policies.NewFlowRequestPolicyImpl;
import rina.ipcprocess.impl.flowallocator.timertasks.TearDownFlowTimerTask;
import rina.registrationmanager.api.RegistrationManager;
import rina.ribdaemon.api.RIBDaemon;
import rina.ribdaemon.api.RIBDaemonException;
import rina.ribdaemon.api.RIBObjectNames;

/**
 * The Flow Allocator is the component of the IPC Process that responds to Allocation API invocations 
 * from Application Processes. It creates and monitors a flow and provides any management over its lifetime.
 * Its only service is to network management.
 * @author eduardgrasa
 *
 */
public class FlowAllocatorInstanceImpl implements FlowAllocatorInstance, CDAPMessageHandler{
	
	private static final Log log = LogFactory.getLog(FlowAllocatorInstanceImpl.class);
	public static final long MAXIMUM_PACKET_LIFETIME_IN_MS = 2000L;
	
	public enum FAIState {NULL, CONNECTION_CREATE_REQUESTED, MESSAGE_TO_PEER_FAI_SENT, 
		APP_NOTIFIED_OF_INCOMING_FLOW, CONNECTION_UPDATE_REQUESTED, FLOW_ALLOCATED, 
		CONNECTION_DESTROY_REQUESTED}
	
	private FAIState state = FAIState.NULL;
	
	/**
	 * The new flow request policy, will translate the allocate request into 
	 * a flow object
	 */
	private NewFlowRequestPolicy newFlowRequestPolicy = null;
	
	/**
	 * The Flow Allocator
	 */
	private FlowAllocator flowAllocator = null;
	
	/**
	 * The portId associated to this Flow Allocator instance
	 */
	private int portId = 0;
	
	/**
	 * The flow object related to this Flow Allocator Instance
	 */
	private Flow flow = null;
	
	/**
	 * The request message;
	 */
	private CDAPMessage requestMessage = null;
	
	/**
	 * The underlying portId to reply to the request message
	 */
	private int underlyingPortId = 0;
	
	/**
	 * Controls if this Flow Allocator instance is operative or not
	 */
	private boolean finished = false;
	
	/**
	 * The timer for this Flow Allocator instance
	 */
	private Timer timer = null;
	
	private CDAPSessionManager cdapSessionManager = null;
	
	private IPCProcess ipcProcess = null;
	
	/**
	 * The name of the flow object associated to this FlowAllocatorInstance
	 */
	private String objectName = null;
	
	/**
	 * The RIBDaemon of the IPC process
	 */
	private RIBDaemon ribDaemon = null;
	
	/**
	 * The class used to encode and decode objects transmitted through CDAP
	 */
	private Encoder encoder = null;
	
	/**
	 * The registrationManager
	 */
	private RegistrationManager registrationManager = null;
	
	/**
	 * Proxy to the IPC Process components in the kernel
	 */
	private KernelIPCProcessSingleton kernelIPCProcess = null;
	
	/**
	 * Proxy to the IPC Manager Daemon
	 */
	private ExtendedIPCManagerSingleton ipcManager = null;
	
	/** 
	 * The event requesting the allocation of a flow
	 */
	private FlowRequestEvent flowRequestEvent = null;
	
	private long allocateResponseMessageHandle = 0;
	
	public FlowAllocatorInstanceImpl(IPCProcess ipcProcess, FlowAllocator flowAllocator, CDAPSessionManager cdapSessionManager, int portId){
		initialize(ipcProcess, flowAllocator, portId);
		this.timer = new Timer();
		this.cdapSessionManager = cdapSessionManager;
		//TODO initialize the newFlowRequestPolicy
		this.newFlowRequestPolicy = new NewFlowRequestPolicyImpl();
		log.debug("Created flow allocator instance to manage the flow identified by portId "+portId);
	}
	
	/**
	 * The flow allocator instance will manage a local flow
	 * @param ipcProcess
	 * @param flowAllocator
	 * @param portId
	 */
	public FlowAllocatorInstanceImpl(IPCProcess ipcProcess, FlowAllocator flowAllocator, int portId){
		initialize(ipcProcess, flowAllocator, portId);
		log.debug("Created flow allocator instance to manage the flow identified by portId "+portId);
	}
	
	private void initialize(IPCProcess ipcProcess, FlowAllocator flowAllocator, int portId){
		this.flowAllocator = flowAllocator;
		this.ipcProcess = ipcProcess;
		this.portId = portId;
		this.ribDaemon = ipcProcess.getRIBDaemon();
		this.encoder = ipcProcess.getEncoder();
		this.registrationManager = ipcProcess.getRegistrationManager();
		this.kernelIPCProcess = rina.getKernelIPCProcess();
		this.ipcManager = rina.getExtendedIPCManager();
	}
	
	public int getPortId(){
		return portId;
	}
	
	public Flow getFlow(){
		return this.flow;
	}
	
	public boolean isFinished(){
		return finished;
	}

	/**
	 * Generate the flow object, create the local DTP and optionally DTCP instances, generate a CDAP 
	 * M_CREATE request with the flow object and send it to the appropriate IPC process (search the 
	 * directory and the directory forwarding table if needed)
	 * @param flowRequestEvent The flow allocation request
	 * @throws IPCException if there are not enough resources to fulfill the allocate request
	 */
	public void submitAllocateRequest(FlowRequestEvent event) throws IPCException {
		this.flowRequestEvent = event;
		flow = newFlowRequestPolicy.generateFlowObject(event, 
				ipcProcess.getDIFInformation().getDifName().getProcessName());
		log.debug("Generated flow object: "+flow.toString());
		
		//1 Check directory to see to what IPC process the CDAP M_CREATE request has to be delivered
		long destinationAddress = registrationManager.getAddress(event.getRemoteApplicationName());
		log.debug("The directory forwarding table returned address "+destinationAddress);
		flow.setDestinationAddress(destinationAddress);
		if (destinationAddress == 0){
			throw new IPCException("Could not find entry in DFT for application "
									+ event.getRemoteApplicationName().getEncodedString());
		}
		
		//2 Check if the destination address is this IPC process (then invoke degenerated form of IPC)
		long sourceAddress = ipcProcess.getAddress().longValue();
		flow.setSourceAddress(sourceAddress);
		flow.setSourcePortId(portId);
		String flowName = ""+sourceAddress+"-"+portId;
		objectName = Flow.FLOW_SET_RIB_OBJECT_NAME + RIBObjectNames.SEPARATOR + flowName;
		if (destinationAddress == sourceAddress){
			// At the moment we don't support allocation of flows between applications at the 
			// same processing system
			throw  new IPCException("Allocation of flows between local applications not supported yet");
		}
		
		//3 Request the creation of the connection(s) in the Kernel
		try{
			this.state = FAIState.CONNECTION_CREATE_REQUESTED;
			kernelIPCProcess.createConnection(flow.getConnections().get(0));
		} catch(Exception ex) {
			throw new IPCException("Problems requesting the kernel to create a connection: " 
					+ ex.getMessage());
		}
	}
	
	private void replyToIPCManager(FlowRequestEvent event, int result) {
		try{
			ipcManager.allocateFlowRequestResult(event, result);
		} catch(Exception ex) {
			log.error("Problems communicating with the IPC Manager Daemon: " + ex.getMessage());
		}
	}
	
	private void releasePortId() {
		try{
			ipcManager.deallocatePortId(portId);
		} catch(Exception ex) {
			log.error("Problems releasing port-id " + portId);
		}
	}
	
	public synchronized void processCreateConnectionResponseEvent(CreateConnectionResponseEvent event) {
		if (state != FAIState.CONNECTION_CREATE_REQUESTED) {
			log.error("Received a process Create Connection Response Event while in "
					+ state + " state. Ignoring it");
			
			return;
		}
		
		if (event.getCepId() < 0) {
			log.error("The EFCP component of the IPC Process could not create a " 
					+  " connection instance: "+event.getCepId());
			replyToIPCManager(flowRequestEvent, -1);
			return;
		}
		
		log.debug("Created connection with cep-id " + event.getCepId());
		flow.getConnections().get(0).setSourceCepId(event.getCepId());
		
		try{
			//5 get the portId of the CDAP session to the destination application process name
			int cdapSessionId = Utils.mapAddressToPortId(flow.getDestinationAddress(), ipcProcess);

			//6 Encode the flow object and send it to the destination IPC process
			ObjectValue objectValue = new ObjectValue();
			objectValue.setByteval(this.encoder.encode(flow));
			CDAPMessage cdapMessage = cdapSessionManager.getCreateObjectRequestMessage(cdapSessionId, null, null, 
					Flow.FLOW_RIB_OBJECT_CLASS, 0, objectName, objectValue, 0, true);
			
			
			underlyingPortId = cdapSessionId;
			requestMessage = cdapMessage;
			state = FAIState.MESSAGE_TO_PEER_FAI_SENT;
			
			ribDaemon.sendMessage(cdapMessage, cdapSessionId, this);
		}catch(Exception ex){
			log.error("Problems sending M_CREATE <Flow> CDAP message to neighbor: " + ex.getMessage());
			flowAllocator.removeFlowAllocatorInstance(portId);
			releasePortId();
			replyToIPCManager(flowRequestEvent, -1);
		}
	}

	/**
	 * When an FAI is created with a Create_Request(Flow) as input, it will inspect the parameters 
	 * first to determine if the requesting Application (Source_Naming_Info) has access to the requested 
	 * Application (Destination_Naming_Info) by inspecting the Access Control parameter.  If not, a 
	 * negative Create_Response primitive will be returned to the requesting FAI. If it does have access, 
	 * the FAI will determine if the policies proposed are acceptable, invoking the NewFlowRequestPolicy.  
	 * If not, a negative Create_Response is sent.  If they are acceptable, the FAI will invoke a 
	 * Allocate_Request.deliver primitive to notify the requested Application that it has an outstanding 
	 * allocation request.  (If the application is not executing, the FAI will cause the application
	 * to be instantiated.)
	 * @param flow
	 * @param portId
	 * @param invokeId
	 * @param flowObjectName
	 */
	public void createFlowRequestMessageReceived(Flow flow, CDAPMessage requestMessage, int underlyingPortId) {
		log.debug("Create flow request received.\n  "+flow.toString());
		this.flow = flow;
		if (this.flow.getDestinationAddress() == 0){
			this.flow.setDestinationAddress(this.ipcProcess.getAddress());
		}
		this.requestMessage = requestMessage;
		this.underlyingPortId = underlyingPortId;
		this.objectName = requestMessage.getObjName();
		flow.setDestinationPortId(portId);
		
		//1 TODO Check if the source application process has access to the destination application process. If not send negative M_CREATE_R 
		//back to the sender IPC process, and housekeeping.
		//Not done in this version, this decision is left to the application 
		//2 TODO If it has, determine if the proposed policies for the flow are acceptable (invoke NewFlowREquestPolicy)
		//Not done in this version, it is assumed that the proposed policies for the flow are acceptable.
		//3 If they are acceptable, the FAI will invoke the Allocate_Request.deliver operation of the destination application process. To do so it will find the
		//IPC Manager and pass it the allocation request.
		try {
			state = FAIState.APP_NOTIFIED_OF_INCOMING_FLOW;
			log.debug("Informing the IPC Manager about an incoming flow allocation request");
			allocateResponseMessageHandle = ipcManager.allocateFlowRequestArrived(flow.getSourceNamingInfo(), 
					flow.getDestinationNamingInfo(), flow.getFlowSpecification(), 
					portId);
		} catch(Exception ex) {
			log.error("Problems informing the IPC Manager about an incoming flow allocation request");
			flowAllocator.removeFlowAllocatorInstance(portId);
			releasePortId();
		}
	}

	/**
	 * When the FAI gets a Allocate_Response from the destination application, it formulates a Create_Response 
	 * on the flow object requested.If the response was positive, the FAI will cause DTP and if required DTCP 
	 * instances to be created to support this allocation. A positive Create_Response Flow is sent to the 
	 * requesting FAI with the connection-endpoint-id and other information provided by the destination FAI. 
	 * The Create_Response is sent to requesting FAI with the necessary information reflecting the existing flow, 
	 * or an indication as to why the flow was refused.  
	 * If the response was negative, the FAI does any necessary housekeeping and terminates.
	 * @param event - the response from the application
	 * @throws IPCException
	 */
	public void submitAllocateResponse(AllocateFlowResponseEvent event) {
		if (state != FAIState.APP_NOTIFIED_OF_INCOMING_FLOW) {
			log.error("Received an allocate response event while not in APP_NOTIFIED_OF_INCOMING_FLOW state. " 
					+ "Current state: " + state );
			return;
		}
		
		CDAPMessage cdapMessage = null;
		if (event.getResult() == 0){
			//1 Request creation of connection
			try {
				kernelIPCProcess.createConnectionArrived(flow.getConnections().get(0));
			} catch (Exception ex) {
				log.error("Problems requesting the kernel to create a connection: "
						+ ex.getMessage());
				
				//Create CDAP response message
				try{
					cdapMessage = cdapSessionManager.getCreateObjectResponseMessage(underlyingPortId, null, 
							requestMessage.getObjClass(), 0, requestMessage.getObjName(), null, -1, 
							"Problems creating connection; "+ex.getMessage(), requestMessage.getInvokeID());
					this.ribDaemon.sendMessage(cdapMessage, underlyingPortId, null);
				}catch(Exception e){
					log.error("Problems requesting the RIB Daemon to send a CDAP message: "+ex.getMessage());
				}
				
				releasePortId();
				
				flowAllocator.removeFlowAllocatorInstance(portId);
				
				try{
					ipcManager.flowDeallocated(portId);
				} catch(Exception e) {
					log.error("Problems communicating with the IPC Manager: " + e.getMessage());
				}
				
				return;
			}
			
			//2 Update Flow state
			state = FAIState.CONNECTION_CREATE_REQUESTED;
		}else{
			releasePortId();
			
			flowAllocator.removeFlowAllocatorInstance(portId);
			
			//Create CDAP response message
			try{
				cdapMessage = cdapSessionManager.getCreateObjectResponseMessage(underlyingPortId, null, 
						requestMessage.getObjClass(), 0, requestMessage.getObjName(), null, -1, 
						"Application rejected the flow: "+event.getResult(), requestMessage.getInvokeID());
				this.ribDaemon.sendMessage(cdapMessage, underlyingPortId, null);
			}catch(Exception ex){
				log.error("Problems requesting the RIB Daemon to send a CDAP message: "+ex.getMessage());
			}
		}
	}
	
	public synchronized void processCreateConnectionResultEvent(CreateConnectionResultEvent event) {
		if (state != FAIState.CONNECTION_CREATE_REQUESTED) {
			log.error("Received an allocate response event while not in APP_NOTIFIED_OF_INCOMING_FLOW state. " 
					+ "Current state: " + state );
			return;
		}
		
		CDAPMessage cdapMessage = null;
		if (event.getSourceCepId() < 0) {
			log.error("The EFCP component of the IPC Process could not create a " 
					+  " connection instance: "+event.getSourceCepId());
			
			//Create CDAP response message
			try{
				cdapMessage = cdapSessionManager.getCreateObjectResponseMessage(underlyingPortId, null, 
						requestMessage.getObjClass(), 0, requestMessage.getObjName(), null, -1, 
						"Problems creating connection; "+event.getSourceCepId(), requestMessage.getInvokeID());
				this.ribDaemon.sendMessage(cdapMessage, underlyingPortId, null);
			}catch(Exception e){
				log.error("Problems requesting the RIB Daemon to send a CDAP message: "+e.getMessage());
			}
			
			releasePortId();
			
			flowAllocator.removeFlowAllocatorInstance(portId);
			
			try{
				ipcManager.flowDeallocated(portId);
			} catch(Exception e) {
				log.error("Problems communicating with the IPC Manager: " + e.getMessage());
			}
			
			return;
		}
		
		//Create CDAP response message
		try{
			ObjectValue objectValue = new ObjectValue();
			objectValue.setByteval(this.encoder.encode(flow));
			cdapMessage = cdapSessionManager.getCreateObjectResponseMessage(underlyingPortId, null, requestMessage.getObjClass(), 
					0, requestMessage.getObjName(), objectValue, 0, null, requestMessage.getInvokeID());
			this.ribDaemon.sendMessage(cdapMessage, underlyingPortId, null);
			this.ribDaemon.create(requestMessage.getObjClass(), requestMessage.getObjName(), this);
		}catch(Exception ex){
			log.error("Problems requesting RIB Daemon to send CDAP Message: "+ex.getMessage());
			releasePortId();
			flowAllocator.removeFlowAllocatorInstance(portId);
			try{
				ipcManager.flowDeallocated(portId);
			} catch(Exception e) {
				log.error("Problems communicating with the IPC Manager: " + e.getMessage());
			}
		}
		
		try{
			ribDaemon.create(Flow.FLOW_RIB_OBJECT_CLASS, objectName, this);
		} catch(Exception ex) {
			log.warn("Error creating Flow Rib object: "+ex.getMessage());
		}
		state = FAIState.FLOW_ALLOCATED;
	}
	
	/**
	 * If the response to the allocate request is negative 
	 * the Allocation invokes the AllocateRetryPolicy. If the AllocateRetryPolicy returns a positive result, a new Create_Flow Request 
	 * is sent and the CreateFlowTimer is reset.  Otherwise, if the AllocateRetryPolicy returns a negative result or the MaxCreateRetries has been exceeded, 
	 * an Allocate_Request.deliver primitive to notify the Application that the flow could not be created. (If the reason was 
	 * "Application Not Found", the primitive will be delivered to the Inter-DIF Directory to search elsewhere.The FAI deletes the DTP and DTCP instances 
	 * it created and does any other housekeeping necessary, before terminating.  If the response is positive, it completes the binding of the DTP-instance 
	 * with this connection-endpoint-id to the requesting Application and invokes a Allocate_Request.submit primitive to notify the requesting Application 
	 * that its allocation request has been satisfied.
	 * @param CDAPMessage
	 * @param CDAPSessionDescriptor
	 */
	public synchronized void createResponse(CDAPMessage cdapMessage, CDAPSessionDescriptor cdapSessionDescriptor) 
			throws RIBDaemonException {
		if (state != FAIState.MESSAGE_TO_PEER_FAI_SENT) {
			log.error("Received CDAP Message while not in MESSAGE_TO_PEER_FAI_SENT state. " 
					+ "Current state is: " + state );
			return;
		}
		
		if (!cdapMessage.getObjName().equals(requestMessage.getObjName())){
			log.error("Expected create flow response message for flow "+requestMessage.getObjName()+
					", but received create flow response message for flow "+cdapMessage.getObjName());
			//TODO, what to do?
			return;
		}
		
		if (cdapMessage.getResult() != 0){
			log.debug("Unsuccessful create flow response message received for flow "+cdapMessage.getObjName());
			releasePortId();
			flowAllocator.removeFlowAllocatorInstance(portId);
			try {
				flowRequestEvent.setPortId(-1);
				ipcManager.allocateFlowRequestResult(flowRequestEvent, cdapMessage.getResult());
			} catch(Exception ex) {
				log.error("Problems communicating with the IPC Manager: "+ex.getMessage());
			}
			return;
		}

		//5 Update the EFCP connection with the destination cep-id
		try {
			if (cdapMessage.getObjValue() != null){
				Flow receivedFlow = (Flow) this.encoder.decode(cdapMessage.getObjValue().getByteval(), Flow.class);
				for(int i=0; i<receivedFlow.getConnections().size(); i++){
					this.flow.getConnections().get(i).setDestCepId(
							receivedFlow.getConnections().get(i).getDestCepId());
				}
			}
			kernelIPCProcess.updateConnection(flow.getConnections().get(0));
		} catch(Exception ex) {
			log.error("Problems requesting kernel to update connection: "+ex.getMessage());
			flowAllocator.removeFlowAllocatorInstance(portId);
			try {
				flowRequestEvent.setPortId(-1);
				ipcManager.allocateFlowRequestResult(flowRequestEvent, cdapMessage.getResult());
			} catch(Exception e) {
				log.error("Problems communicating with the IPC Manager: "+e.getMessage());
			}
			return;
		}

		//Update flow state
		this.state = FAIState.CONNECTION_UPDATE_REQUESTED;

		//7 Create the Flow object in the RIB, start a socket reader and deliver the response to the application
		/*log.debug("Successfull create flow message response received for flow "+cdapMessage.getObjName()+".\n "+this.flow.toString());
			this.ribDaemon.create(Flow.FLOW_RIB_OBJECT_CLASS, objectName, this);*/
	}
	
	public void processUpdateConnectionResponseEvent(UpdateConnectionResponseEvent event) {
		if (state != FAIState.CONNECTION_UPDATE_REQUESTED) {
			log.error("Received CDAP Message while not in MESSAGE_TO_PEER_FAI_SENT state. " 
					+ "Current state is: " + state );
			return;
		}
		
		if (event.getResult() != 0) {
			log.error("The kernel denied the update of a connection: "  + event.getResult());
			
			releasePortId();
			flowAllocator.removeFlowAllocatorInstance(portId);
			
			try {
				flowRequestEvent.setPortId(-1);
				ipcManager.allocateFlowRequestResult(flowRequestEvent, event.getResult());
			} catch(Exception ex) {
				log.error("Problems communicating with the IPC Manager");
			}
			
			return;
		}
		
		log.debug("Successfull create flow message response received for flow "+objectName+".\n "+this.flow.toString());
		try {
			ribDaemon.create(Flow.FLOW_RIB_OBJECT_CLASS, objectName, this);
		} catch(Exception ex) {
			log.warn("Problems requesting the RIB Daemon to create a RIB object: "+ex.getMessage());
		}
		
		state = FAIState.FLOW_ALLOCATED;
	}
	
	/**
	 * When a deallocate primitive is invoked, it is passed to the FAI responsible for that port-id.  
	 * The FAI sends an M_DELETE request CDAP PDU on the Flow object referencing the destination port-id, deletes the local 
	 * binding between the Application and the DTP-instance and waits for a response.  (Note that 
	 * the DTP and DTCP if it exists will be deleted automatically after 2MPL)
	 * @throws IPCException
	 */
	public void submitDeallocate(FlowDeallocateRequestEvent event) {
		if (state != FAIState.FLOW_ALLOCATED) {
			log.error("Received deallocate request while not in FLOW_ALLOCATED state. " 
					+ "Current state is: " + state );
			return;
		}

		try{
			//1 Send M_DELETE
			try{
				ObjectValue objectValue = new ObjectValue();
				objectValue.setByteval(this.encoder.encode(flow));
				requestMessage = cdapSessionManager.getDeleteObjectRequestMessage(
						underlyingPortId, null, null, "flow", 0, requestMessage.getObjName(), null, 0, false); 
				this.ribDaemon.sendMessage(requestMessage, underlyingPortId, null);
			}catch(Exception ex){
				log.error("Problems sending M_DELETE flow request");
			}

			//2 Update flow state
			this.flow.setState(State.WAITING_2_MPL_BEFORE_TEARING_DOWN);

			//3 Wait 2*MPL before tearing down the flow
			TearDownFlowTimerTask timerTask = new TearDownFlowTimerTask(this, this.objectName, true);
			timer.schedule(timerTask, TearDownFlowTimerTask.DELAY);
		}catch(Exception ex){
			log.error("Problems processing flow deallocation request" +ex.getMessage());
		}
	}

	/**
	 * When this PDU is received by the FAI with this port-id, the FAI invokes a Deallocate.deliver to notify the local Application, 
	 * deletes the binding between the Application and the local DTP-instance, and sends a Delete_Response indicating the result.
	 * @param cdapMessage
	 * @param underlyingPortId
	 */
	public void deleteFlowRequestMessageReceived(CDAPMessage cdapMessage, int underlyingPortId){
		if (state != FAIState.FLOW_ALLOCATED) {
			log.error("Received deallocate request while not in FLOW_ALLOCATED state. " 
					+ "Current state is: " + state );
			return;
		}
		
		//1 Update flow state
		this.flow.setState(State.WAITING_2_MPL_BEFORE_TEARING_DOWN);
		
		//3 Set timer
		TearDownFlowTimerTask timerTask = new TearDownFlowTimerTask(this, this.objectName, false);
		timer.schedule(timerTask, TearDownFlowTimerTask.DELAY);
	}
	
	public synchronized void destroyFlowAllocatorInstance(String flowObjectName, boolean requestor) {
		if (flow.getState() != State.WAITING_2_MPL_BEFORE_TEARING_DOWN) {
			log.error("Invoked destroy flow allocator instance while not in "
					+ "WAITING_2_MPL_BEFORE_TEARING_DOWN. State: " + flow.getState());
		}
		
		releasePortId();
		
		flowAllocator.removeFlowAllocatorInstance(portId);
		
		try {
			kernelIPCProcess.destroyConnection(flow.getConnections().get(0));
		} catch (Exception ex) {
			log.error("Problems requesting the kernel to destroy a connection: " 
					+ ex.getMessage());
		}
	}

	public void deleteResponse(CDAPMessage cdapMessage, CDAPSessionDescriptor cdapSessionDescriptor) throws RIBDaemonException{
	}

	public void readResponse(CDAPMessage arg0, CDAPSessionDescriptor arg1)
			throws RIBDaemonException {
		// TODO Auto-generated method stub
		
	}

	public void startResponse(CDAPMessage arg0, CDAPSessionDescriptor arg1)
			throws RIBDaemonException {
		// TODO Auto-generated method stub
		
	}

	public void stopResponse(CDAPMessage arg0, CDAPSessionDescriptor arg1)
			throws RIBDaemonException {
		// TODO Auto-generated method stub
		
	}

	public void writeResponse(CDAPMessage arg0, CDAPSessionDescriptor arg1)
			throws RIBDaemonException {
		// TODO Auto-generated method stub
		
	}

	public void cancelReadResponse(CDAPMessage arg0, CDAPSessionDescriptor arg1)
			throws RIBDaemonException {
		// TODO Auto-generated method stub
		
	}
}
