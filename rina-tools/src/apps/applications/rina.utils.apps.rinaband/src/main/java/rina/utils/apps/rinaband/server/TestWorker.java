package rina.utils.apps.rinaband.server;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import eu.irati.librina.Flow;
import eu.irati.librina.FlowState;
import eu.irati.librina.rina;
import rina.utils.apps.rinaband.TestInformation;
import rina.utils.apps.rinaband.utils.FlowReader;
import rina.utils.apps.rinaband.utils.SDUListener;

/**
 * Deals with an individual data flow, keeping the statistics of 
 * the received SDUs.
 * @author eduardgrasa
 *
 */
public class TestWorker implements SDUListener{
	
	/**
	 * The information of this test
	 */
	private TestInformation testInformation = null;
	
	/**
	 * The flow from the RINABand client
	 */
	private Flow flow = null;
	
	/**
	 * True if the test has started
	 */
	private boolean started = false;
	
	/**
	 * The number of SDUs delivered
	 */
	private int deliveredSDUs = 0;
	
	/**
	 * The time of the first SDU Received
	 */
	private long timeOfFirstSDUReceived =0;
	
	/**
	 * The controller of this test
	 */
	private TestController testController = null;
	
	private FlowReader flowReader = null;
	
	private static final Log log = LogFactory.getLog(TestWorker.class);
	
	public TestWorker(TestInformation testInformation, Flow flow, TestController testController){
		this.testInformation = testInformation;
		this.flow = flow;
		this.testController = testController;
	}
	
	public void setFlowReader(FlowReader flowReader){
		this.flowReader = flowReader;
	}
	
	public FlowReader getFlowReader(){
		return this.flowReader;
	}
	
	/**
	 * If the server has to send SDUs, execute a new thread that does so
	 */
	public void execute(){
		this.started = true;
		
		if (this.testInformation.isServerSendsSDUs()){
			SDUSender sduSender = new SDUSender(this.testInformation, this.flow, this.testController);
			RINABandServer.executeRunnable(sduSender);
		}
	}
	
	/**
	 * Stop the execution by deallocating the flow
	 */
	public void abort(){
		if (this.started){
			try{
				this.flowReader.stop();
				
				if (this.flow.getState() == FlowState.FLOW_ALLOCATED){
					rina.getIpcManager().requestFlowDeallocation(this.flow.getPortId());
				}
			}catch(Exception ex){
			}
		}
		
		this.started = false;
	}

	public void sduDelivered(byte[] sdu) {
		deliveredSDUs ++;
		if (timeOfFirstSDUReceived == 0){
			timeOfFirstSDUReceived = System.nanoTime();
			testController.setFirstSDUReveived(System.currentTimeMillis());
		}
		if (deliveredSDUs == this.testInformation.getNumberOfSDUs()){
			long time = System.nanoTime() - timeOfFirstSDUReceived;
			testController.setLastSDUReceived(System.currentTimeMillis());
			long sentSDUsperSecond = 1000L*1000L*1000L*this.testInformation.getNumberOfSDUs()/time;
			log.info("Flow at portId "+flow.getPortId()+": Received SDUs per second: "+sentSDUsperSecond);
			log.info("Flow at portId "+flow.getPortId()+": Received KiloBytes per second (KBps): "
					+sentSDUsperSecond*this.testInformation.getSduSize()/1024);
		}
	}

}
