package rina.utils.apps.rinaband.server;

import java.util.Hashtable;
import java.util.Iterator;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Timer;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import eu.irati.librina.ApplicationProcessNamingInformation;
import eu.irati.librina.ApplicationRegistration;
import eu.irati.librina.ApplicationRegistrationInformation;
import eu.irati.librina.ApplicationRegistrationType;
import eu.irati.librina.Flow;
import eu.irati.librina.FlowDeallocatedEvent;
import eu.irati.librina.FlowRequestEvent;
import eu.irati.librina.IPCManagerSingleton;
import eu.irati.librina.RegisterApplicationResponseEvent;
import eu.irati.librina.UnregisterApplicationResponseEvent;
import eu.irati.librina.rina;

import rina.cdap.api.CDAPSessionManager;
import rina.cdap.api.message.CDAPMessage;
import rina.cdap.api.message.ObjectValue;
import rina.utils.apps.rinaband.StatisticsInformation;
import rina.utils.apps.rinaband.TestInformation;
import rina.utils.apps.rinaband.protobuf.RINABandStatisticsMessageEncoder;
import rina.utils.apps.rinaband.protobuf.RINABandTestMessageEncoder;
import rina.utils.apps.rinaband.utils.ApplicationRegistrationListener;
import rina.utils.apps.rinaband.utils.FlowAcceptor;
import rina.utils.apps.rinaband.utils.FlowDeallocationListener;
import rina.utils.apps.rinaband.utils.FlowReader;
import rina.utils.apps.rinaband.utils.IPCEventConsumer;
import rina.utils.apps.rinaband.utils.SDUListener;

/**
 * Controls the negotiation of test parameters and 
 * the execution of a single test
 * @author eduardgrasa
 *
 */
public class TestController implements SDUListener, FlowAcceptor, 
	FlowDeallocationListener, ApplicationRegistrationListener{
	
	private enum State {WAIT_CREATE, WAIT_START, EXECUTING, WAIT_STOP, COMPLETED};
	
	/**
	 * The state of the test
	 */
	private State state = State.WAIT_CREATE;
	
	/**
	 * The information of this test
	 */
	private TestInformation testInformation = null;
	
	/**
	 * The APNamingInfo associated to the data AE of the RINABand application
	 */
	private ApplicationProcessNamingInformation dataApNamingInfo = null;
	
	/**
	 * The CDAPSessionManager
	 */
	private CDAPSessionManager cdapSessionManager = null;
	
	/**
	 * The flow from the RINABand client
	 */
	private Flow flow = null;
	
	/**
	 * The map of allocated flows, with the 
	 * classes that deal with each individual flow
	 */
	private Map<Integer, TestWorker> allocatedFlows = null;
	
	/**
	 * The stop test message
	 */
	private CDAPMessage stopTestMessage = null;
	
	private IPCEventConsumer ipcEventConsumer = null;
	
	private ApplicationRegistration dataAERegistration = null;
	
	private ApplicationProcessNamingInformation difName = null;
	
	private FlowReader flowReader = null;
	
	/**
	 * Epoch times are in miliseconds
	 */
	private long epochTimeFirstSDUReceived = 0;
	private long epochTimeLastSDUReceived = 0;
	private int completedReceives = 0;
	
	private long epochTimeFirstSDUSent = 0;
	private long epochTimeLastSDUSent = 0;
	private int completedSends = 0;
	
	private static final Log log = LogFactory.getLog(TestController.class);
	
	private Timer timer = null;
	private CDAPMessage storedCDAPMessage = null;
	
	public TestController(ApplicationProcessNamingInformation dataApNamingInfo, 
			ApplicationProcessNamingInformation difName, Flow flow,
			CDAPSessionManager cdapSessionManager, IPCEventConsumer ipcEventConsumer){
		this.dataApNamingInfo = dataApNamingInfo;
		this.difName = difName;
		this.cdapSessionManager = cdapSessionManager;
		this.flow = flow;
		this.allocatedFlows = new Hashtable<Integer, TestWorker>();
		this.ipcEventConsumer = ipcEventConsumer;
		this.timer = new Timer();
	}
	
	public void setFlowReader(FlowReader flowReader){
		this.flowReader = flowReader;
	}
	
	public FlowReader getFlowReader(){
		return this.flowReader;
	}

	public synchronized void sduDelivered(byte[] sdu) {
		try{
			CDAPMessage cdapMessage = this.cdapSessionManager.decodeCDAPMessage(sdu);
			System.out.println("Received CDAP Message: "+cdapMessage.toString());
			switch(cdapMessage.getOpCode()){
			case M_CREATE:
				handleCreateMessageReceived(cdapMessage);
				break;
			case M_START:
				handleStartMessageReceived(cdapMessage);
				break;
			case M_STOP:
				handleStopMessageReceived(cdapMessage);
				break;
			default:
				log.info("Received CDAP Message with wrong opcode, ignoring it.");
			}
		}catch(Exception ex){
			log.info("Error decoding CDAP Message.");
			ex.printStackTrace();
		}
	}
	
	/**
	 * Check the data in the TestInformation object, change the values 
	 * that we do not agree with and register the Data AE that will 
	 * receive the test flow Allocations
	 * @param cdapMessage
	 */
	private void handleCreateMessageReceived(CDAPMessage cdapMessage){
		if (this.state != State.WAIT_CREATE){
			log.info("Received CREATE Test message while not in WAIT_CREATE state." + 
			" Ignoring it.");
			return;
		}
		
		ObjectValue objectValue = cdapMessage.getObjValue();
		if (objectValue == null || objectValue.getByteval() == null){
			log.info("The create message did not contain an object value. Ignoring the message");
			return;
		}
		
		try{
			//1 Decode and update the testInformation object
			this.testInformation = RINABandTestMessageEncoder.decode(objectValue.getByteval());
			this.testInformation.setAei(""+flow.getPortId());
			if (this.testInformation.getNumberOfFlows() > RINABandServer.MAX_NUMBER_OF_FLOWS){
				this.testInformation.setNumberOfFlows(RINABandServer.MAX_NUMBER_OF_FLOWS);
			}
			if (this.testInformation.getNumberOfSDUs() > RINABandServer.MAX_SDUS_PER_FLOW){
				this.testInformation.setNumberOfSDUs(RINABandServer.MAX_SDUS_PER_FLOW);
			}
			if (this.testInformation.getSduSize() > RINABandServer.MAX_SDU_SIZE_IN_BYTES){
				this.testInformation.setSduSize(RINABandServer.MAX_SDU_SIZE_IN_BYTES);
			}
			
			//2 Update the DATA AE and request its registration
			dataApNamingInfo.setEntityInstance(this.testInformation.getAei());
			ipcEventConsumer.addApplicationRegistrationListener(this, dataApNamingInfo);
			ApplicationRegistrationInformation appRegInfo = 
					new ApplicationRegistrationInformation(ApplicationRegistrationType.APPLICATION_REGISTRATION_SINGLE_DIF);
			appRegInfo.setAppName(dataApNamingInfo);
			appRegInfo.setDifName(difName);
			rina.getIpcManager().requestApplicationRegistration(appRegInfo);
			storedCDAPMessage = cdapMessage;
		}catch(Exception ex){
			log.info("Error handling CREATE Test message.");
			ex.printStackTrace();
		}
	}
	
	@Override
	public synchronized void dispatchApplicationRegistrationResponseEvent(
			RegisterApplicationResponseEvent event) {
		if (event.getResult() != 0) {
			log.error("Problems registering data AE: "+ event.getApplicationName().toString() 
					+ ". Error code is "+event.getResult());
			try {
				rina.getIpcManager().withdrawPendingRegistration(event.getSequenceNumber());
			} catch(Exception ex) {
				log.error(ex.getMessage());
			}
			
			//FIXME properly clean up and cancel the test
			return;
		}
		
		try{
			dataAERegistration = rina.getIpcManager().commitPendingRegistration(event.getSequenceNumber(), 
					event.getDIFName());
		} catch(Exception ex){
			log.error(ex.getMessage());
			//FIXME what to do now?
		}
		
		log.info("Registered data AE " + event.getApplicationName().toString() 
			+ " to DIF " + difName.getProcessName());
		
		ipcEventConsumer.addFlowAcceptor(this, dataApNamingInfo);

		//3 Reply and update state
		try {
			CDAPMessage replyMessage = storedCDAPMessage.getReplyMessage();
			ObjectValue objectValue = new ObjectValue();
			objectValue.setByteval(RINABandTestMessageEncoder.encode(this.testInformation));
			replyMessage.setObjValue(objectValue);
			sendCDAPMessage(replyMessage);
			this.state = State.WAIT_START;
			log.info("Waiting to START a new test with the following parameters.");
			log.info(this.testInformation.toString());
		} catch (Exception ex){
			log.info("Error handling CREATE Test message.");
			ex.printStackTrace();
		}
	}

	@Override
	public void dispatchApplicationUnregistrationResponseEvent(
			UnregisterApplicationResponseEvent event) {
		boolean success = false;
		
		if (event.getResult() == 0){
			success = true;
		}
		
		try {
			rina.getIpcManager().appUnregistrationResult(event.getSequenceNumber(), success);
		} catch (Exception ex) {
			log.error(ex.getMessage());
		}	
		
		if (event.getResult() == 0){
			log.info("Data AE " + event.getApplicationName().toString() 
					+ " successfully unregistered from DIF " + event.getDIFName().getProcessName());
		} else {
			log.info("Problems unregistering data AE from DIF " + 
					event.getDIFName().getProcessName() + ". Error code: "+event.getResult());
		}
	}
	
	/**
	 * Start the test. Be prepared to accept new flows, receive data and/or to create 
	 * new flows and write data
	 * @param cdapMessage
	 */
	private void handleStartMessageReceived(CDAPMessage cdapMessage){
		if (this.state != State.WAIT_START){
			log.info("Received START Test message while not in WAIT_START state." + 
			" Ignoring it.");
			return;
		}
		
		Iterator<Entry<Integer, TestWorker>> iterator = allocatedFlows.entrySet().iterator();
		while(iterator.hasNext()){
			iterator.next().getValue().execute();
		}
		
		this.state = State.EXECUTING;
		timer.schedule(new CancelTestTimerTask(this), 20*1000);
		log.info("Started test execution");
	}
	
	private void handleStopMessageReceived(CDAPMessage cdapMessage){
		if (this.state == State.EXECUTING){
			this.stopTestMessage = cdapMessage;
			log.info("Received STOP Test message while still in EXECUTING state." + 
			" Storing it and waiting to send/receive all test data.");
			return;
		}else if (this.state != State.WAIT_STOP){
			log.info("Received STOP Test message while not in EXECUTING or WAIT_STOP state, ignoring it");
			return;
		}
		
		this.stopTestMessage = cdapMessage;
		this.printStatsAndFinishTest();
	}
	
	public synchronized void setFirstSDUSent(long epochTime){
		if (this.epochTimeFirstSDUSent == 0){
			this.epochTimeFirstSDUSent = epochTime;
		}
	}
	
	public synchronized void setFirstSDUReveived(long epochTime){
		if (this.epochTimeFirstSDUReceived == 0){
			this.epochTimeFirstSDUReceived = epochTime;
		}
	}
	
	public synchronized void setLastSDUSent(long epochTime){
		this.completedSends++;
		if (this.completedSends == this.testInformation.getNumberOfFlows()){
			this.epochTimeLastSDUSent = epochTime;
			if (!this.testInformation.isClientSendsSDUs() || this.completedReceives == this.testInformation.getNumberOfFlows()){
				this.changeToWaitStopState();
			}
		}
	}
	
	public synchronized void setLastSDUReceived(long epochTime){
		this.completedReceives++;
		if (this.completedReceives == this.testInformation.getNumberOfFlows()){
			this.epochTimeLastSDUReceived = epochTime;
			if (!this.testInformation.isServerSendsSDUs() || this.completedSends == this.testInformation.getNumberOfFlows()){
				this.changeToWaitStopState();
			}
		}
	}
	
	private void changeToWaitStopState(){
		this.state = State.WAIT_STOP;
		if (this.stopTestMessage != null){
			printStatsAndFinishTest();
		}
	}
	
	/**
	 * The test maximum duration has expired, but the 
	 * test may have not finished yet
	 */
	public synchronized void timerExpired(){
		if (this.state == State.COMPLETED){
			return;
		}
		
		this.state = State.WAIT_STOP;
		if (this.stopTestMessage != null){
			printStatsAndFinishTest();
		}
	}
	
	private void printStatsAndFinishTest(){
		//1 Write statistics as response and print the stats
		try{
			//Update the statistics and send the M_STOP_R message
			StatisticsInformation statsInformation = RINABandStatisticsMessageEncoder.decode(this.stopTestMessage.getObjValue().getByteval());
			if (this.testInformation.isClientSendsSDUs()){
				statsInformation.setServerTimeFirstSDUReceived(this.epochTimeFirstSDUReceived*1000L);
				statsInformation.setServerTimeLastSDUReceived(this.epochTimeLastSDUReceived*1000L);
			}
			if (this.testInformation.isServerSendsSDUs()){
				statsInformation.setServerTimeFirstSDUSent(this.epochTimeFirstSDUSent*1000L);
				statsInformation.setServerTimeLastSDUSent(this.epochTimeLastSDUSent*1000L);
			}
			log.info(statsInformation.toString());
			ObjectValue objectValue = new ObjectValue();
			objectValue.setByteval(RINABandStatisticsMessageEncoder.encode(statsInformation));
			CDAPMessage responseMessage = CDAPMessage.getStopObjectResponseMessage(null, 0, null, this.stopTestMessage.getInvokeID());
			responseMessage.setObjClass(this.stopTestMessage.getObjClass());
			responseMessage.setObjName(this.stopTestMessage.getObjName());
			responseMessage.setObjValue(objectValue);
			sendCDAPMessage(responseMessage);
			
			//Print aggregate statistics
			long averageClientServerDelay = 0L;
			long averageServerClientDelay = 0L;
			log.info("Aggregate bandwidth:");
			if (this.testInformation.isClientSendsSDUs()){
				long aggregateReceivedSDUsPerSecond = 1000L*this.testInformation.getNumberOfFlows()*
					this.testInformation.getNumberOfSDUs()/(this.epochTimeLastSDUReceived-this.epochTimeFirstSDUReceived);
				log.info("Aggregate received SDUs per second: "+aggregateReceivedSDUsPerSecond);
				averageClientServerDelay = ((this.epochTimeFirstSDUReceived - statsInformation.getClientTimeFirstSDUSent()/1000L) + 
						(this.epochTimeLastSDUReceived - statsInformation.getClientTimeLastSDUSent()/1000L))/2;
				log.info("Aggregate received KiloBytes per second (KBps): "+ aggregateReceivedSDUsPerSecond*this.testInformation.getSduSize()/1024);
				log.info("Aggregate received Megabits per second (Mbps): "+ aggregateReceivedSDUsPerSecond*this.testInformation.getSduSize()*8/(1024*1024));
			}
			if (this.testInformation.isServerSendsSDUs()){
				long aggregateSentSDUsPerSecond = 1000L*1000L*this.testInformation.getNumberOfFlows()*
					this.testInformation.getNumberOfSDUs()/(statsInformation.getClientTimeLastSDUReceived()-statsInformation.getClientTimeFirstSDUReceived());
				averageServerClientDelay = ((statsInformation.getClientTimeFirstSDUReceived()/1000L - this.epochTimeFirstSDUSent) + 
						(statsInformation.getClientTimeLastSDUReceived()/1000L - this.epochTimeLastSDUSent))/2;
				log.info("Aggregate sent SDUs per second: "+aggregateSentSDUsPerSecond);
				log.info("Aggregate sent KiloBytes per second (KBps): "+ aggregateSentSDUsPerSecond*this.testInformation.getSduSize()/1024);
				log.info("Aggregate sent Megabits per second (Mbps): "+ aggregateSentSDUsPerSecond*this.testInformation.getSduSize()*8/(1024*1024));
			}
			long rttInMs = 0L;
			if (this.testInformation.isClientSendsSDUs() && this.testInformation.isServerSendsSDUs()){
				rttInMs = averageClientServerDelay + averageServerClientDelay;
			}else if (this.testInformation.isClientSendsSDUs()){
				rttInMs = averageClientServerDelay*2;
			}else{
				rttInMs = averageServerClientDelay*2;
			}
			log.info("Estimated round-trip time (RTT) in ms: "+rttInMs);
		}catch(Exception ex){
			log.info("Problems returning STOP RESPONSE message");
			ex.printStackTrace();
		}
		
		//2 Cancel the timer and update the state
		this.state = State.COMPLETED;
		this.timer.cancel();
	}

	/**
	 * Called when a new data flow has been allocated
	 */
	public synchronized void flowAllocated(Flow flow) {
		if (this.state != State.WAIT_START){
			log.info("New flow allocated, but we're not in the WAIT_START state. Requesting deallocation.");
			try{
				rina.getIpcManager().requestFlowDeallocation(flow.getPortId());
			}catch(Exception ex){
				ex.printStackTrace();
			}
			return;
		}
		
		TestWorker testWorker = new TestWorker(this.testInformation, flow, this);
		FlowReader flowReader = new FlowReader(flow, testWorker, this.testInformation.getSduSize());
		testWorker.setFlowReader(flowReader);
		RINABandServer.executeRunnable(flowReader);
		this.allocatedFlows.put(new Integer(flow.getPortId()), testWorker);
		ipcEventConsumer.addFlowDeallocationListener(this, flow.getPortId());
		log.info("Data flow with portId "+flow.getPortId()+ " allocated");
	}

	/**
	 * Called when an existing data flow has been deallocated
	 */
	public synchronized void flowDeallocated(int portId) {
		TestWorker testWorker = this.allocatedFlows.remove(new Integer(portId));
		testWorker.getFlowReader().stop();
		log.info("Data flow with portId "+portId+ " deallocated");
		ipcEventConsumer.removeFlowDeallocationListener(portId);
		
		try{
			rina.getIpcManager().flowDeallocated(portId);
		} catch (Exception ex){
			log.warn("Error updating librina data structures for flow " + portId 
					+ ". " + ex.getMessage());
		}
		
		if (this.allocatedFlows.size() == 0){
			//Cancel the registration of the data AE
			try{
				rina.getIpcManager().requestApplicationUnregistration(dataApNamingInfo, 
						dataAERegistration.getDIFNames().getFirst());
				ipcEventConsumer.removeFlowAcceptor(dataApNamingInfo);
			}catch(Exception ex){
				log.info("Problems unregistering data AE");
				ex.printStackTrace();
			}
		}
	}
	
	/**
	 * Decide when a flow can be accepted
	 */
	public synchronized void dispatchFlowRequestEvent(FlowRequestEvent event){
		IPCManagerSingleton ipcManager = rina.getIpcManager();
		
		try{
			Flow flow = ipcManager.allocateFlowResponse(event, 0, true);
			flowAllocated(flow);
		}catch(Exception ex){
			ex.printStackTrace();
		}
	}
	
	public synchronized void dispatchFlowDeallocatedEvent(FlowDeallocatedEvent event){
		flowDeallocated(event.getPortId());
	}

	private void sendCDAPMessage(CDAPMessage cdapMessage) throws Exception{
		byte[] sdu = cdapSessionManager.encodeCDAPMessage(cdapMessage);
		flow.writeSDU(sdu, sdu.length);
	}
	
}
