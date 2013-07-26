package rina.utils.apps.rinaband.client;

import java.util.ArrayList;
import java.util.List;
import java.util.Timer;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

import eu.irati.librina.ApplicationProcessNamingInformation;
import eu.irati.librina.Flow;
import eu.irati.librina.FlowSpecification;
import eu.irati.librina.rina;

import rina.cdap.api.CDAPSessionManager;
import rina.cdap.api.message.CDAPMessage;
import rina.cdap.api.message.ObjectValue;
import rina.cdap.impl.CDAPSessionManagerImpl;
import rina.cdap.impl.googleprotobuf.GoogleProtocolBufWireMessageProviderFactory;
import rina.utils.apps.rinaband.Main;
import rina.utils.apps.rinaband.StatisticsInformation;
import rina.utils.apps.rinaband.TestInformation;
import rina.utils.apps.rinaband.protobuf.RINABandStatisticsMessageEncoder;
import rina.utils.apps.rinaband.protobuf.RINABandTestMessageEncoder;
import rina.utils.apps.rinaband.utils.FlowReader;
import rina.utils.apps.rinaband.utils.SDUListener;

/**
 * Implements the behavior of a RINABand Client
 * @author eduardgrasa
 *
 */
public class RINABandClient implements SDUListener{
	
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
	
	public RINABandClient(TestInformation testInformation, ApplicationProcessNamingInformation controlApNamingInfo, 
			ApplicationProcessNamingInformation dataApNamingInfo){
		this.testInformation = testInformation;
		this.controlApNamingInfo = controlApNamingInfo;
		this.dataApNamingInfo = dataApNamingInfo;
		this.clientApNamingInfo = new ApplicationProcessNamingInformation("rina.utils.apps.rinabandclient", "");
		this.cdapSessionManager = new CDAPSessionManagerImpl(new GoogleProtocolBufWireMessageProviderFactory());
		this.testWorkers = new ArrayList<TestWorker>();
		this.executorService = Executors.newCachedThreadPool();
	}
	
	public void execute(){
		try{
			//1 Allocate a flow to the RINABand Server control AE
			FlowSpecification qosSpec = new FlowSpecification();
			this.controlFlow = rina.getIpcManager().allocateFlowRequest(this.clientApNamingInfo, this.controlApNamingInfo, qosSpec);
			
			//2 Update the state
			this.state = State.WAIT_CREATE_R;
			
			//3 Send the create test message
			ObjectValue objectValue = new ObjectValue();
			objectValue.setByteval(RINABandTestMessageEncoder.encode(this.testInformation));
			CDAPMessage cdapMessage = CDAPMessage.getCreateObjectRequestMessage(
					null, null, TEST_OBJECT_CLASS, 0, TEST_OBJECT_NAME, objectValue, 0);
			cdapMessage.setInvokeID(1);
			byte[] sdu = this.cdapSessionManager.encodeCDAPMessage(cdapMessage);
			this.controlFlow.writeSDU(sdu, sdu.length);
			System.out.println("Requested a new test with the following parameters:");
			System.out.println(this.testInformation.toString());
		}catch(Exception ex){
			ex.printStackTrace();
			abortTest("Problems requesting test");
		}
	}

	/**
	 * Called when an SDU is available on the control flow
	 */
	public void sduDelivered(byte[] sdu, int numBytes){
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
			
			System.out.println("Starting a new test with the following parameters:");
			System.out.println(testInformation.toString());
			
			//Setup all the flows
			Flow flow = null;
			TestWorker testWorker = null;
			long before = 0;
			int retries = 0;
			FlowSpecification qosSpec = new FlowSpecification();
			/*if (testInformation.getQos().equals(Main.RELIABLE)){
				qosSpec.setQosCubeId(2);
			}else{
				qosSpec.setQosCubeId(1);
			}*/
			this.timer = new Timer();
			for(int i=0; i<testInformation.getNumberOfFlows(); i++){
				try{
					testWorker = new TestWorker(testInformation, this, timer);
					before = System.currentTimeMillis();
					retries = 0;
					
					//Retry flow allocation for up to 3 times in case the RINABand data AE registration update had not 
					//reached the directory of the IPC process running in the local system
					while(retries < 3){
						try{
							flow = rina.getIpcManager().allocateFlowRequest(clientApNamingInfo, dataApNamingInfo, qosSpec);
							testWorker.setFlow(flow, System.currentTimeMillis() - before);
							testWorkers.add(testWorker);
							FlowReader flowReader = new FlowReader(flow, testWorker, testInformation.getSduSize());
							executorService.execute(flowReader);
							break;
						}catch(Exception ex){
							if (ex.getMessage().indexOf("Could not find an entry in the directory forwarding table") != 0){
								System.out.println("Flow request failed, trying again");
								retries++;
							}else{
								break;
							}
						}
					}
				}catch(Exception ex){
					System.out.println("Flow setup failed");
					ex.printStackTrace();
				}
			}
			state = State.EXECUTING;
			
			System.out.println("Allocated "+testWorkers.size()+" flows. Executing the test ...");
			
			//2 Tell the server to start the test
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
			//1 Unallocate the data flows
			TestWorker currentWorker = null;
			long before = 0;
			for(int i=0; i<this.testWorkers.size(); i++){
				currentWorker = this.testWorkers.get(i);
				before = System.currentTimeMillis();
				currentWorker.abortTest();
				currentWorker.getStatistics().setFlowTearDownTimeInMillis(System.currentTimeMillis()-before);
			}
			
			//2 Tell the server to STOP the test
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
			System.out.println("Requested the RINABand server to stop the test and to get statistics back");
			this.state = State.STOP_REQUESTED;
		}catch(Exception ex){
			ex.printStackTrace();
			abortTest("Problems cleaning up the test resources");
		}
	}
		
	private void handleStopResponseReceived(CDAPMessage stopResponse){
		try{
			System.out.println("Test successfully completed!");
			StatisticsInformation statsInformation = RINABandStatisticsMessageEncoder.decode(stopResponse.getObjValue().getByteval());
			System.out.println(statsInformation.toString());
			
			//1 Print the individual statistics
			TestWorker currentWorker = null;
			for(int i=0; i<this.testWorkers.size(); i++){
				currentWorker = this.testWorkers.get(i);
				System.out.println("Statistics of flow "+currentWorker.getFlow().getPortId());
				System.out.println("Flow allocation time (ms): "+currentWorker.getStatistics().getFlowSetupTimeInMillis());
				System.out.println("Flow deallocation time (ms): "+currentWorker.getStatistics().getFlowTearDownTimeInMillis());
				System.out.println("Sent SDUs: "+currentWorker.getStatistics().getSentSDUS());
				System.out.println("Sent SDUs per second: "+currentWorker.getStatistics().getSentSDUsPerSecond());
				System.out.println("Sent KiloBytes per second (KBps): "+
						currentWorker.getStatistics().getSentSDUsPerSecond()*this.testInformation.getSduSize()/1024);
				System.out.println("Recveived SDUs: "+currentWorker.getStatistics().getReceivedSDUs());
				System.out.println("Received SDUs per second: "+currentWorker.getStatistics().getReceivedSDUsPerSecond());
				System.out.println("Received KiloBytes per second (KBps): "+
						currentWorker.getStatistics().getReceivedSDUsPerSecond()*this.testInformation.getSduSize()/1024);
				System.out.println("% of received SDUs lost: "+100*currentWorker.getStatistics().getReceivedSDUsLost()/this.testInformation.getNumberOfSDUs());
				System.out.println();
			}
			
			//2 Print the aggregated statistics
			System.out.println("Mean statistics");
			System.out.println("Mean flow allocation time (ms): "+ this.getMean(Type.FLOW_ALLOCATION));
			System.out.println("Mean flow deallocation time (ms): "+ this.getMean(Type.FLOW_DEALLOCATION));
			long averageClientServerDelay = 0L;
			long averageServerClientDelay = 0L;
			if (this.testInformation.isClientSendsSDUs()){
				long meanSentSdus = this.getMean(Type.SENT_SDUS);
				System.out.println("Mean sent SDUs per second: "+ meanSentSdus);
				System.out.println("Mean sent KiloBytes per second (KBps): "+ meanSentSdus*this.testInformation.getSduSize()/1024);
				averageClientServerDelay = ((statsInformation.getServerTimeFirstSDUReceived()/1000L - this.epochTimeFirstSDUSent) + 
						(statsInformation.getServerTimeLastSDUReceived()/1000L - this.epochTimeLastSDUSent))/2;
			}
			if (this.testInformation.isServerSendsSDUs()){
				long meanReceivedSdus = this.getMean(Type.RECEIVED_SDUS);
				System.out.println("Mean received SDUs per second: "+ meanReceivedSdus);
				System.out.println("Mean received KiloBytes per second (KBps): "+ meanReceivedSdus*this.testInformation.getSduSize()/1024);
				averageServerClientDelay = ((this.epochTimeFirstSDUReceived - statsInformation.getServerTimeFirstSDUSent()/1000L) + 
						(this.epochTimeLastSDUReceived - statsInformation.getServerTimeLastSDUSent()/1000L))/2;
			}
			System.out.println();
			System.out.println("Aggregate bandwidth:");
			if (this.testInformation.isClientSendsSDUs()){
				long aggregateSentSDUsPerSecond = 1000L*1000L*this.testInformation.getNumberOfFlows()*
					this.testInformation.getNumberOfSDUs()/(statsInformation.getServerTimeLastSDUReceived()-statsInformation.getServerTimeFirstSDUReceived());
				System.out.println("Aggregate sent SDUs per second: "+aggregateSentSDUsPerSecond);
				System.out.println("Aggregate sent KiloBytes per second (KBps): "+ aggregateSentSDUsPerSecond*this.testInformation.getSduSize()/1024);
				System.out.println("Aggregate sent Megabits per second (Mbps): "+ aggregateSentSDUsPerSecond*this.testInformation.getSduSize()*8/(1024*1024));
			}
			if (this.testInformation.isServerSendsSDUs()){
				long aggregateReceivedSDUsPerSecond = 1000L*this.testInformation.getNumberOfFlows()*
					this.testInformation.getNumberOfSDUs()/(this.epochTimeLastSDUReceived-this.epochTimeFirstSDUReceived);
				System.out.println("Aggregate received SDUs per second: "+aggregateReceivedSDUsPerSecond);
				System.out.println("Aggregate received KiloBytes per second (KBps): "+ aggregateReceivedSDUsPerSecond*this.testInformation.getSduSize()/1024);
				System.out.println("Aggregate received Megabits per second (Mbps): "+ aggregateReceivedSDUsPerSecond*this.testInformation.getSduSize()*8/(1024*1024));
			}
			
			long rttInMs = 0L;
			if (this.testInformation.isClientSendsSDUs() && this.testInformation.isServerSendsSDUs()){
				rttInMs = averageClientServerDelay + averageServerClientDelay;
			}else if (this.testInformation.isClientSendsSDUs()){
				rttInMs = averageClientServerDelay*2;
			}else{
				rttInMs = averageServerClientDelay*2;
			}
			System.out.println("Estimated round-trip time (RTT) in ms: "+rttInMs);
			
			//3 Deallocate the control flow
			rina.getIpcManager().deallocateFlow(this.controlFlow.getPortId());
			this.state = State.COMPLETED;
			
			//4 Exit
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
		
		System.out.println(message + ". Aborting the test.");
		System.exit(-1);
	}
	
}
