package rina.utils.apps.rinaband.client;

import java.util.ArrayList;
import java.util.List;
import java.util.Timer;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import eu.irati.librina.AllocateFlowRequestResultEvent;
import eu.irati.librina.ApplicationProcessNamingInformation;
import eu.irati.librina.Flow;
import eu.irati.librina.FlowSpecification;
import eu.irati.librina.rina;

import rina.utils.LogHelper;
import rina.cdap.api.CDAPSessionManager;
import rina.cdap.api.message.CDAPMessage;
import rina.cdap.api.message.ObjectValue;
import rina.cdap.impl.CDAPSessionManagerImpl;
import rina.cdap.impl.googleprotobuf.GoogleProtocolBufWireMessageProviderFactory;
import rina.utils.apps.rinaband.StatisticsInformation;
import rina.utils.apps.rinaband.TestInformation;
import rina.utils.apps.rinaband.protobuf.RINABandStatisticsMessageEncoder;
import rina.utils.apps.rinaband.protobuf.RINABandTestMessageEncoder;
import rina.utils.apps.rinaband.utils.FlowAllocationListener;
import rina.utils.apps.rinaband.utils.FlowReader;
import rina.utils.apps.rinaband.utils.IPCEventConsumer;
import rina.utils.apps.rinaband.utils.SDUListener;

/**
 * Implements the behavior of a RINABand Client
 * @author eduardgrasa
 *
 */
public class RINABandClient implements SDUListener, FlowAllocationListener{
	
	public static final String TEST_OBJECT_CLASS = "RINAband test";
	public static final String TEST_OBJECT_NAME = "/rina/utils/apps/rinaband/test";
	public static final String STATISTICS_OBJECT_CLASS = "RINAband statistics";
	public static final String STATISTICS_OBJECT_NAME = "/rina/utils/apps/rinaband/statistics";

	private enum State {NULL, WAIT_CREATE_R, EXECUTING, STOP_REQUESTED, COMPLETED};
	
	private enum Type{FLOW_ALLOCATION, FLOW_DEALLOCATION, SENT_SDUS, RECEIVED_SDUS};
	
	/**
	 * The state of the client
	 */
	private State state = State.NULL;
	
	/**
	 * The data about the test to carry out
	 */
	private TestInformation testInformation = null;
	
	/**
	 * The APNamingInfo associated to the control AE of the RINABand application
	 */
	private ApplicationProcessNamingInformation controlApNamingInfo = null;
	
	/**
	 * The APNamingInfo associated to the data AE of the RINABand application
	 */
	private ApplicationProcessNamingInformation dataApNamingInfo = null;
	
	/**
	 * The client AP Naming Information
	 */
	private ApplicationProcessNamingInformation clientApNamingInfo = null;
	
	/**
	 * The flow to the RINABand server control AE
	 */
	private Flow controlFlow = null;
	
	/**
	 * The CDAP Parser
	 */
	private CDAPSessionManager cdapSessionManager = null;
	
	/**
	 * The test workers
	 */
	private List<TestWorker> testWorkers = null;
	
	/**
	 * The thread pool
	 */
	private ExecutorService executorService = null;
	
	/**
	 * The number of test workers that have completed their individual flow test
	 */
	private int completedTestWorkers = 0;
	
	/**
	 * Epoch times are in miliseconds
	 */
	private long epochTimeFirstSDUReceived = 0;
	private long epochTimeLastSDUReceived = 0;
	private int completedSends = 0;
	
	private long epochTimeFirstSDUSent = 0;
	private long epochTimeLastSDUSent = 0;
	private int completedReceives = 0;
	private Timer timer = null;
	
	private static final Log log = LogFactory.getLog(RINABandClient.class);
	
	private long controlFlowHandle = -1;
	private List<Long> dataFlowHandles = null;
	
	private IPCEventConsumer ipcEventConsumer = null;
	
	public RINABandClient(TestInformation testInformation, ApplicationProcessNamingInformation controlApNamingInfo, 
			ApplicationProcessNamingInformation dataApNamingInfo){
		try {
			rina.initialize(LogHelper.getLibrinaLogLevel(), 
					LogHelper.getLibrinaLogFile());
		} catch(Exception ex){
			log.error("Problems initializing librina, exiting: "+ex.getMessage());
			System.exit(-1);
		}
		
		this.testInformation = testInformation;
		this.controlApNamingInfo = controlApNamingInfo;
		this.dataApNamingInfo = dataApNamingInfo;
		this.clientApNamingInfo = new ApplicationProcessNamingInformation("rina.utils.apps.rinabandclient", "");
		this.cdapSessionManager = new CDAPSessionManagerImpl(new GoogleProtocolBufWireMessageProviderFactory());
		this.testWorkers = new ArrayList<TestWorker>();
		this.executorService = Executors.newCachedThreadPool();
		ipcEventConsumer = new IPCEventConsumer();
		executorService.execute(ipcEventConsumer);
		dataFlowHandles = new ArrayList<Long>();
	}
	
	public void execute(){
		try{
			//1 Request a flow allocationto the RINABand server control AE
			FlowSpecification qosSpec = new FlowSpecification();
			controlFlowHandle = rina.getIpcManager().requestFlowAllocation(
					this.clientApNamingInfo, this.controlApNamingInfo, qosSpec);
			ipcEventConsumer.addFlowAllocationListener(this, controlFlowHandle);
		}catch(Exception ex){
			ex.printStackTrace();
			abortTest("Problems requesting test");
		}
	}
	
	@Override
	public synchronized void dispatchAllocateFlowRequestResultEvent(
			AllocateFlowRequestResultEvent event) {
		ipcEventConsumer.removeFlowAllocationListener(event.getSequenceNumber());

		if (event.getSequenceNumber() == controlFlowHandle){
			if (event.getPortId() > 0){
				processControlFlowAllocated(event);
			} else {
				log.error("Problems allocating flow to control AE: " + controlApNamingInfo.toString());
				try{
					rina.getIpcManager().withdrawPendingFlow(event.getSequenceNumber());
				} catch (Exception ex) {
					log.error(ex.getMessage());
				}
				abortTest("Problems requesting test");
			}
		} else if (dataFlowHandles.contains(event.getSequenceNumber())){
			dataFlowHandles.remove(event.getSequenceNumber());
			processDataFlowAllocated(event);

			if (dataFlowHandles.size() == 0) {
				state = State.EXECUTING;

				log.info("Allocated "+testWorkers.size()+" flows. Executing the test ...");

				//2 Tell the server to start the test
				try{
					CDAPMessage startTestMessage = CDAPMessage.getStartObjectRequestMessage(
							null, null, TEST_OBJECT_CLASS, null, 0, TEST_OBJECT_NAME, 0);
					byte[] sdu = cdapSessionManager.encodeCDAPMessage(startTestMessage);
					controlFlow.writeSDU(sdu, sdu.length);

					//3 If the client has to send SDUs, execute the TestWorkers in separate threads
					if (testInformation.isClientSendsSDUs()){
						for(int i=0; i<testWorkers.size(); i++){
							executorService.execute(testWorkers.get(i));
						}
					}
				}catch(Exception ex){
					ex.printStackTrace();
				}
			}
		}
	}
	
	private void processControlFlowAllocated(AllocateFlowRequestResultEvent event){
		try{
			//1 Commit flow
			controlFlow = rina.getIpcManager().commitPendingFlow(event.getSequenceNumber(), 
					event.getPortId(), event.getDifName());
			
			//2 Start flowReader
			FlowReader flowReader = new FlowReader(this.controlFlow, this, 10000);
			this.executorService.execute(flowReader);
			
			//3 Update the state
			this.state = State.WAIT_CREATE_R;
			
			//4 Send the create test message
			ObjectValue objectValue = new ObjectValue();
			objectValue.setByteval(RINABandTestMessageEncoder.encode(this.testInformation));
			CDAPMessage cdapMessage = CDAPMessage.getCreateObjectRequestMessage(
					null, null, TEST_OBJECT_CLASS, 0, TEST_OBJECT_NAME, objectValue, 0);
			cdapMessage.setInvokeID(1);
			byte[] sdu = this.cdapSessionManager.encodeCDAPMessage(cdapMessage);
			this.controlFlow.writeSDU(sdu, sdu.length);
			log.info("Requested a new test with the following parameters:");
			log.info(this.testInformation.toString());
		} catch(Exception ex){
			log.error(ex.getMessage());
			abortTest("Problems requesting test");
		}
	}

	private void processDataFlowAllocated(AllocateFlowRequestResultEvent event){
		try{
			if (event.getPortId() > 0){
				//1 Commit flow
				Flow flow = rina.getIpcManager().commitPendingFlow(event.getSequenceNumber(), 
						event.getPortId(), event.getDifName());
				
				//2 Start test worker
				TestWorker testWorker = new TestWorker(testInformation, this, timer);
				testWorker.setFlow(flow, 0);
				testWorkers.add(testWorker);
				FlowReader flowReader = new FlowReader(flow, testWorker, testInformation.getSduSize());
				testWorker.setFlowReader(flowReader);
				executorService.execute(flowReader);
			} else {
				rina.getIpcManager().withdrawPendingFlow(event.getSequenceNumber());
				log.error("Allocation of flow with portId " + event.getPortId()+ " failed");
			}
		} catch(Exception ex){
			log.error(ex.getMessage());
			abortTest("Problems requesting test");
		}
	}

	/**
	 * Called when an SDU is available on the control flow
	 */
	public void sduDelivered(byte[] sdu){
		try{
			CDAPMessage cdapMessage = this.cdapSessionManager.decodeCDAPMessage(sdu);
			System.out.println("Received a CDAP Message! "+cdapMessage.toString());
			switch(cdapMessage.getOpCode()){
			case M_CREATE_R:
				handleCreateResponseReceived(cdapMessage);
				break;
			case M_STOP_R:
				handleStopResponseReceived(cdapMessage);
				break;
			default:
				abortTest("Received wrong CDAP Message");
			}
		}catch(Exception ex){
			ex.printStackTrace();
			abortTest("Problems decoding CDAP Message");
		}
	}
	
	/**
	 * Called when a create response message has been received
	 * @param cdapMessage
	 */
	private void handleCreateResponseReceived(CDAPMessage cdapMessage){
		if (state != State.WAIT_CREATE_R){
			abortTest("Received CREATE Response message while not in WAIT_CREATE_R state");
		}
		
		if (cdapMessage.getResult() != 0){
			abortTest("Received unsuccessful CREATE Response message. Reason: "+cdapMessage.getResultReason());
		}
		
		ObjectValue objectValue = cdapMessage.getObjValue();
		if (objectValue == null || objectValue.getByteval() == null){
			abortTest("Object value is null");
		}
		
		try{
			//1 Update the testInformation and dataApNamingInfo
			testInformation = RINABandTestMessageEncoder.decode(objectValue.getByteval());
			dataApNamingInfo.setEntityInstance(testInformation.getAei());
			
			log.info("Starting a new test with the following parameters:");
			log.info(testInformation.toString());
			
			//Setup all the flows
			FlowSpecification qosSpec = new FlowSpecification();
			
			//Give time to the RINABand data AE registration to reach our directory
			try{
				Thread.sleep(500);
			}catch(Exception ex){
				log.error(ex.getMessage());
			}
			
			this.timer = new Timer();
			long handle = -1;
			for(int i=0; i<testInformation.getNumberOfFlows(); i++){
				try{
					handle = rina.getIpcManager().requestFlowAllocation(clientApNamingInfo, dataApNamingInfo, qosSpec);
					ipcEventConsumer.addFlowAllocationListener(this, handle);
					dataFlowHandles.add(handle);
				}catch(Exception ex){
					log.error("Flow request failed: "+ex.getMessage());
				}
			}
		}catch(Exception ex){
			ex.printStackTrace();
			abortTest("Problems processing M_CREATE Test response");
		}
	}
	
	public synchronized void setLastSDUSent(long epochTime){
		this.completedSends++;
		if (this.completedSends == this.testInformation.getNumberOfFlows()){
			this.epochTimeLastSDUSent = epochTime;
		}
	}
	
	public synchronized void setLastSDUReceived(long epochTime){
		this.completedReceives++;
		if (this.completedReceives == this.testInformation.getNumberOfFlows()){
			this.epochTimeLastSDUReceived = epochTime;
		}
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
	
	/**
	 * Called by the test worker when he has finished sending/receiving SDUs
	 * @param testWorker
	 */
	public synchronized void testCompleted(TestWorker testWorker){
		this.completedTestWorkers++;
		if (this.completedTestWorkers < this.testWorkers.size()){
			return;
		}
		
		//All the tests have completed
		try{
			//1 Tell the server to STOP the test
			StatisticsInformation statisticsInformation = new StatisticsInformation();
			if (this.testInformation.isServerSendsSDUs()){
				statisticsInformation.setClientTimeLastSDUSent(this.epochTimeLastSDUSent*1000L);
				statisticsInformation.setClientTimeLastSDUReceived(this.epochTimeLastSDUReceived*1000L);
			}
			if (this.testInformation.isClientSendsSDUs()){
				statisticsInformation.setClientTimeFirstSDUSent(this.epochTimeFirstSDUSent*1000L);
				statisticsInformation.setClientTimeFirstSDUReceived(this.epochTimeFirstSDUReceived*1000L);
			}
			ObjectValue objectValue = new ObjectValue();
			objectValue.setByteval(RINABandStatisticsMessageEncoder.encode(statisticsInformation));
			CDAPMessage stopTestMessage = CDAPMessage.getStopObjectRequestMessage(
					null, null, STATISTICS_OBJECT_CLASS, objectValue, 0, STATISTICS_OBJECT_NAME, 0);
			stopTestMessage.setInvokeID(1);
			byte[] sdu = this.cdapSessionManager.encodeCDAPMessage(stopTestMessage);
			this.controlFlow.writeSDU(sdu, sdu.length);
			log.info("Requested the RINABand server to stop the test and to get statistics back");
			this.state = State.STOP_REQUESTED;
		}catch(Exception ex){
			ex.printStackTrace();
			abortTest("Problems cleaning up the test resources");
		}
	}
		
	private void handleStopResponseReceived(CDAPMessage stopResponse){
		try{
			log.info("Received test stop confirmation from server, deallocating the flows...");
			//1 Unallocate the data flows
			TestWorker currentWorker = null;
			long before = 0;
			for(int i=0; i<this.testWorkers.size(); i++){
				currentWorker = this.testWorkers.get(i);
				before = System.currentTimeMillis();
				currentWorker.abortTest();
				currentWorker.getStatistics().setFlowTearDownTimeInMillis(System.currentTimeMillis()-before);
			}
			
			log.info("Test successfully completed!");
			StatisticsInformation statsInformation = RINABandStatisticsMessageEncoder.decode(stopResponse.getObjValue().getByteval());
			System.out.println(statsInformation.toString());
			
			//2 Print the individual statistics
			for(int i=0; i<this.testWorkers.size(); i++){
				currentWorker = this.testWorkers.get(i);
				log.info("Statistics of flow "+currentWorker.getFlow().getPortId());
				log.info("Flow allocation time (ms): "+currentWorker.getStatistics().getFlowSetupTimeInMillis());
				log.info("Flow deallocation time (ms): "+currentWorker.getStatistics().getFlowTearDownTimeInMillis());
				log.info("Sent SDUs: "+currentWorker.getStatistics().getSentSDUS());
				log.info("Sent SDUs per second: "+currentWorker.getStatistics().getSentSDUsPerSecond());
				log.info("Sent KiloBytes per second (KBps): "+
						currentWorker.getStatistics().getSentSDUsPerSecond()*this.testInformation.getSduSize()/1024);
				log.info("Recveived SDUs: "+currentWorker.getStatistics().getReceivedSDUs());
				log.info("Received SDUs per second: "+currentWorker.getStatistics().getReceivedSDUsPerSecond());
				log.info("Received KiloBytes per second (KBps): "+
						currentWorker.getStatistics().getReceivedSDUsPerSecond()*this.testInformation.getSduSize()/1024);
				log.info("% of received SDUs lost: "+100*currentWorker.getStatistics().getReceivedSDUsLost()/this.testInformation.getNumberOfSDUs());
				log.info("");
			}
			
			//3 Print the aggregated statistics
			log.info("Mean statistics");
			log.info("Mean flow allocation time (ms): "+ this.getMean(Type.FLOW_ALLOCATION));
			log.info("Mean flow deallocation time (ms): "+ this.getMean(Type.FLOW_DEALLOCATION));
			long averageClientServerDelay = 0L;
			long averageServerClientDelay = 0L;
			if (this.testInformation.isClientSendsSDUs()){
				long meanSentSdus = this.getMean(Type.SENT_SDUS);
				log.info("Mean sent SDUs per second: "+ meanSentSdus);
				log.info("Mean sent KiloBytes per second (KBps): "+ meanSentSdus*this.testInformation.getSduSize()/1024);
				averageClientServerDelay = ((statsInformation.getServerTimeFirstSDUReceived()/1000L - this.epochTimeFirstSDUSent) + 
						(statsInformation.getServerTimeLastSDUReceived()/1000L - this.epochTimeLastSDUSent))/2;
			}
			if (this.testInformation.isServerSendsSDUs()){
				long meanReceivedSdus = this.getMean(Type.RECEIVED_SDUS);
				log.info("Mean received SDUs per second: "+ meanReceivedSdus);
				log.info("Mean received KiloBytes per second (KBps): "+ meanReceivedSdus*this.testInformation.getSduSize()/1024);
				averageServerClientDelay = ((this.epochTimeFirstSDUReceived - statsInformation.getServerTimeFirstSDUSent()/1000L) + 
						(this.epochTimeLastSDUReceived - statsInformation.getServerTimeLastSDUSent()/1000L))/2;
			}
			log.info("");
			log.info("Aggregate bandwidth:");
			if (this.testInformation.isClientSendsSDUs()){
				long aggregateSentSDUsPerSecond = 1000L*1000L*this.testInformation.getNumberOfFlows()*
					this.testInformation.getNumberOfSDUs()/(statsInformation.getServerTimeLastSDUReceived()-statsInformation.getServerTimeFirstSDUReceived());
				log.info("Aggregate sent SDUs per second: "+aggregateSentSDUsPerSecond);
				log.info("Aggregate sent KiloBytes per second (KBps): "+ aggregateSentSDUsPerSecond*this.testInformation.getSduSize()/1024);
				log.info("Aggregate sent Megabits per second (Mbps): "+ aggregateSentSDUsPerSecond*this.testInformation.getSduSize()*8/(1024*1024));
			}
			if (this.testInformation.isServerSendsSDUs()){
				long aggregateReceivedSDUsPerSecond = 1000L*this.testInformation.getNumberOfFlows()*
					this.testInformation.getNumberOfSDUs()/(this.epochTimeLastSDUReceived-this.epochTimeFirstSDUReceived);
				log.info("Aggregate received SDUs per second: "+aggregateReceivedSDUsPerSecond);
				log.info("Aggregate received KiloBytes per second (KBps): "+ aggregateReceivedSDUsPerSecond*this.testInformation.getSduSize()/1024);
				log.info("Aggregate received Megabits per second (Mbps): "+ aggregateReceivedSDUsPerSecond*this.testInformation.getSduSize()*8/(1024*1024));
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
			
			//4 Deallocate the control flow
			rina.getIpcManager().requestFlowDeallocation(this.controlFlow.getPortId());
			this.state = State.COMPLETED;
			
			//5 Exit
			System.exit(0);
		}catch(Exception ex){
			ex.printStackTrace();
			abortTest("Problems cleaning up the test resources");
		}
	}
	
	private long getMean(Type type){
		long accumulatedValue = 0;
		for(int i=0; i<this.testWorkers.size(); i++){
			switch(type){
			case FLOW_ALLOCATION: 
				accumulatedValue = accumulatedValue + 
				this.testWorkers.get(i).getStatistics().getFlowSetupTimeInMillis();
				break;
			case FLOW_DEALLOCATION: 
				accumulatedValue = accumulatedValue + 
				this.testWorkers.get(i).getStatistics().getFlowTearDownTimeInMillis();
				break;
			case SENT_SDUS:
				accumulatedValue = accumulatedValue + 
				this.testWorkers.get(i).getStatistics().getSentSDUsPerSecond();
				break;
			case RECEIVED_SDUS:
				accumulatedValue = accumulatedValue + 
				this.testWorkers.get(i).getStatistics().getReceivedSDUsPerSecond();
				break;
			}
			
		}
		
		return accumulatedValue/this.testWorkers.size();
	}
	
	
	private void abortTest(String message){
		if (this.state == State.EXECUTING){
			for(int i=0; i<testWorkers.size(); i++){
				this.testWorkers.get(i).abortTest();
			}
		}
		
		log.info(message + ". Aborting the test.");
		System.exit(-1);
	}
}
