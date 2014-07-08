package rina.utils.apps.rinaband.server;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import eu.irati.librina.Flow;
import eu.irati.librina.FlowState;
import eu.irati.librina.rina;
import rina.utils.apps.rinaband.TestInformation;
import rina.utils.apps.rinaband.generator.BoringSDUGenerator;
import rina.utils.apps.rinaband.generator.IncrementSDUGenerator;
import rina.utils.apps.rinaband.generator.SDUGenerator;

/**
 * Sends a number of SDUs through a flow
 * @author eduardgrasa
 *
 */
public class SDUSender implements Runnable {
	
	/**
	 * The information of this test
	 */
	private TestInformation testInformation = null;
	
	/**
	 * The flow from the RINABand client
	 */
	private Flow flow = null;
	
	/**
	 * The number of SDUs generated
	 */
	private int generatedSDUs = 0;
	
	/**
	 * The class that generates the SDUs
	 */
	private SDUGenerator sduGenerator = null;
	
	/**
	 * The controller of this test
	 */
	private TestController testController = null;
	
	private static final Log log = LogFactory.getLog(SDUSender.class);
	
	public SDUSender(TestInformation testInformation, Flow flow, TestController testController){
		this.testInformation = testInformation;
		this.flow = flow;
		this.testController = testController;
		if (this.testInformation.getPattern().equals(SDUGenerator.NONE_PATTERN)){
			sduGenerator = new BoringSDUGenerator(this.testInformation.getSduSize());
		}else if (this.testInformation.getPattern().equals(SDUGenerator.INCREMENT_PATTERN)){
			sduGenerator = new IncrementSDUGenerator(this.testInformation.getSduSize());
		}
	}

	public void run() {
		long before = System.nanoTime();
		byte[] sdu;
		
		int numberOfSdus = testInformation.getNumberOfSDUs();
		for(generatedSDUs=0; generatedSDUs<numberOfSdus; generatedSDUs++){
			try{
				sdu = sduGenerator.getNextSDU();
				flow.writeSDU(sdu, sdu.length);
				if (generatedSDUs == 0){
					this.testController.setFirstSDUSent(System.currentTimeMillis());
				}
			}catch(Exception ex){
				System.out.println("SDU Sender of flow "+flow.getPortId()+": Error writing SDU. Canceling operation");
				ex.printStackTrace();
				try{
					if (flow.getState() == FlowState.FLOW_ALLOCATED){
						rina.getIpcManager().requestFlowDeallocation(flow.getPortId());
					}
				}catch(Exception e){
				}
				break;
			}
		}
		
		long time = System.nanoTime() - before;
		this.testController.setLastSDUSent(System.currentTimeMillis());
		long sentSDUsperSecond = 1000L*1000L*1000L*numberOfSdus/time;
		log.info("Flow at portId "+flow.getPortId()+": Sent SDUs per second: "+sentSDUsperSecond);
		log.info("Flow at portId "+flow.getPortId()+": Sent KiloBytes per second (KBps): "
				+sentSDUsperSecond*this.testInformation.getSduSize()/1024);
	}
}
