package rina.utils.apps.rinaband.client;

import java.util.Timer;

import eu.irati.librina.Flow;
import eu.irati.librina.FlowState;
import eu.irati.librina.rina;

import rina.utils.apps.rinaband.TestInformation;
import rina.utils.apps.rinaband.generator.BoringSDUGenerator;
import rina.utils.apps.rinaband.generator.IncrementSDUGenerator;
import rina.utils.apps.rinaband.generator.SDUGenerator;
import rina.utils.apps.rinaband.utils.FlowReader;
import rina.utils.apps.rinaband.utils.SDUListener;

public class TestWorker implements Runnable, SDUListener{

	/**
	 * The data about the test to carry out
	 */
	private TestInformation testInformation = null;
	
	/**
	 * The flow
	 */
	private Flow flow = null;
	
	/**
	 * A pointer to the RINABandClient class
	 */
	private RINABandClient rinaBandClient = null;
	
	/**
	 * The statistics of this test of the flow
	 */
	private TestFlowStatistics statistics = null;
	
	/**
	 * The object lock
	 */
	private Object lock = null;
	
	/**
	 * The number of SDUs received
	 */
	private int receivedSDUs = 0;
	
	/**
	 * The time when the first SDU was received
	 */
	private long nanoTimeOfFirstSDUReceived = 0;
	
	/**
	 * The epoch time when the first SDU was received (in milliseconds)
	 */
	private long epochTimeOfFirstSDUReceived = 0;
	
	/**
	 * The time when the first SDU was received
	 */
	private long timeOfFirstSDUSent = 0;
	
	/**
	 * True if we have received all the SDUs
	 */
	private boolean receiveCompleted = false;
	
	/**
	 * True if we have sent all the SDUs
	 */
	private boolean sendCompleted = false;
	
	/**
	 * The class that generates the SDUs
	 */
	private SDUGenerator sduGenerator = null;
	
	/**
	 * The test declared dead timer task
	 */
	private TestDeclaredDeadTimerTask testDeadTimerTask = null;
	
	private Timer timer = null;
	
	private FlowReader flowReader = null;
	
	public TestWorker(TestInformation testInformation, RINABandClient rinaBandClient, Timer timer){
		this.testInformation = testInformation;
		this.rinaBandClient = rinaBandClient;
		this.statistics = new TestFlowStatistics();
		this.lock = new Object();
		this.timer = timer;
		
		if (!this.testInformation.isClientSendsSDUs()){
			this.sendCompleted = true;
		}
		
		if (!this.testInformation.isServerSendsSDUs()){
			this.receiveCompleted = true;
		}else{
			this.scheduleTestDeadTimerTask();
		}
		
		if (this.testInformation.getPattern().equals(SDUGenerator.NONE_PATTERN)){
			sduGenerator = new BoringSDUGenerator(this.testInformation.getSduSize());
		}else if (this.testInformation.getPattern().equals(SDUGenerator.INCREMENT_PATTERN)){
			sduGenerator = new IncrementSDUGenerator(this.testInformation.getSduSize());
		}
	}
	
	public void setFlowReader(FlowReader flowReader){
		this.flowReader = flowReader;
	}
	
	public FlowReader getFlowReader(){
		return this.flowReader;
	}
	
	private void scheduleTestDeadTimerTask(){
		testDeadTimerTask = new TestDeclaredDeadTimerTask(this);
		timer.schedule(testDeadTimerTask, TestDeclaredDeadTimerTask.DEFAULT_DELAY_IN_MS);
	}

	public void setFlow(Flow flow, long flowSetupTimeInMillis){
		this.flow = flow;
		this.statistics.setFlowSetupTimeInMillis(flowSetupTimeInMillis);
	}
	
	public Flow getFlow(){
		return flow;
	}
	
	public TestFlowStatistics getStatistics(){
		return this.statistics;
	}
	
	public void abortTest(){
		flowReader.stop();
		
		if (this.flow.getState() == FlowState.FLOW_ALLOCATED){
			try{
				rina.getIpcManager().requestFlowDeallocation(
						this.flow.getPortId());
			}catch(Exception ex){
			}
		}
	}
	
	/**
	 * If this is called it is because this worker needs to send a number of SDUs through the flow.
	 */
	public void run() {
		if (!this.testInformation.isClientSendsSDUs()){
			return;
		}
		
		long sdusSent = 0;
		byte[] sdu;
		
		this.timeOfFirstSDUSent = System.currentTimeMillis();
		rinaBandClient.setFirstSDUSent(this.timeOfFirstSDUSent);
		for(int i=0; i<this.testInformation.getNumberOfSDUs(); i++){
			try{
				sdu = this.sduGenerator.getNextSDU();
				flow.writeSDU(sdu, sdu.length);
				sdusSent++;
			}catch(Exception ex){
				System.out.println("Problems sending SDU");
				synchronized(lock){
					this.sendCompleted = true;
					if (this.receiveCompleted){
						this.rinaBandClient.testCompleted(this);
					}
				}
				return;
			}
		}
		
		long currentTime = System.currentTimeMillis();
		long totalTimeInMilis = (currentTime - this.timeOfFirstSDUSent);
		if (totalTimeInMilis == 0){
			totalTimeInMilis ++;
		}
		this.rinaBandClient.setLastSDUSent(currentTime);
		synchronized(lock){
			this.statistics.setSentSDUS(sdusSent);
			this.statistics.setSentSDUsPerSecond(1000L*sdusSent/totalTimeInMilis);
			this.sendCompleted = true;
			if (this.receiveCompleted){
				this.rinaBandClient.testCompleted(this);
			}
		}
	}

	/**
	 * Called when an sdu is received through the flow
	 */
	public void sduDelivered(byte[] sdu) {
		testDeadTimerTask.cancel();
		
		if (this.receivedSDUs == 0){
			this.nanoTimeOfFirstSDUReceived = System.nanoTime();
			this.epochTimeOfFirstSDUReceived = System.currentTimeMillis();
			this.rinaBandClient.setFirstSDUReveived(this.epochTimeOfFirstSDUReceived);
		}
		
		this.receivedSDUs++;
		if (this.receivedSDUs == this.testInformation.getNumberOfSDUs()){
			this.processLastSDUReceived();
		}else{
			this.scheduleTestDeadTimerTask();
		}
	}
	
	/**
	 * Called by the test declared dead timer if no SDU has been received in 
	 * X ms.
	 */
	public void notWaitingToReceiveMoreSDUs(){
		this.processLastSDUReceived();
	}
	
	private void processLastSDUReceived(){
		long currentTimeInNanos = System.nanoTime();
		long epochTime = System.currentTimeMillis();
		long totalTimeInNanos = (currentTimeInNanos - this.nanoTimeOfFirstSDUReceived);
		this.rinaBandClient.setLastSDUReceived(epochTime);
		synchronized(lock){
			this.statistics.setReceivedSDUs(this.receivedSDUs);
			this.statistics.setReceivedSDUsPerSecond(1000L*1000L*1000L*this.receivedSDUs/totalTimeInNanos);
			this.statistics.setReceivedSDUsLost(this.testInformation.getNumberOfSDUs() - this.receivedSDUs);
			this.receiveCompleted = true;
			if (this.sendCompleted){
				this.rinaBandClient.testCompleted(this);
			}
		}
	}
}
