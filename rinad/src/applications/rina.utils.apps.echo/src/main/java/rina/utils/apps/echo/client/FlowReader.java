package rina.utils.apps.echo.client;

import java.util.Calendar;
import java.util.Timer;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import rina.utils.apps.echo.TestInformation;
import rina.utils.apps.echo.utils.FlowDeallocationListener;

import eu.irati.librina.Flow;
import eu.irati.librina.FlowDeallocatedEvent;
import eu.irati.librina.FlowDeallocationException;
import eu.irati.librina.rina;

/**
 * Reads sdus from a flow
 * @author eduardgrasa
 */
public class FlowReader implements Runnable, FlowDeallocationListener{
	
	private Flow flow = null;
	private TestInformation testInformation = null;
	private boolean stop;
	private Timer timer = null;
	
	private static final Log log = LogFactory.getLog(FlowReader.class);
	
	public FlowReader(Flow flow, TestInformation testInformation){
		this.flow = flow;
		this.testInformation = testInformation;
		this.timer = new Timer();
	}

	@Override
	public void run() {
		byte[] buffer = new byte[EchoClient.MAX_SDU_SIZE];
		
		TestDeclaredDeadTimerTask timerTask = new TestDeclaredDeadTimerTask(this);
		timer.schedule(timerTask, 20*1000);
		
		while(!isStopped()){
			try{
				flow.readSDU(buffer, buffer.length);
				testInformation.sduReceived();
				if (testInformation.receivedAllSDUs()) {
					testInformation.setLastSDUReceivedTime(
							Calendar.getInstance().getTimeInMillis());
					long testDuration = testInformation.getLastSDUReceivedTime() 
							- testInformation.getFirstSDUSentTime();
					if (testDuration == 0) {
						testDuration = 1;
					}
					log.info("Test completed, sent and received " + 
							testInformation.getNumberOfSDUs() + " of " 
							+ testInformation.getSduSize() + 
							" in " +testDuration + " ms.");
					long bandwidthInBps = 1000*testInformation.getNumberOfSDUs()*testInformation.getSduSize()/testDuration;
					log.info("Send and received at " + bandwidthInBps 
							+ " Bytes per second ( " +bandwidthInBps*8/1024 + " Kbps)");
					stop();
				}
			}catch(Exception ex){
				log.error("Problems reading SDU from flow "+flow.getPortId());
				stop();
			}
		}
		
		terminateReader();
		System.exit(0);
	}
	
	public synchronized void stop(){
		if (!stop) {
			log.info("Requesting reader of flow "+flow.getPortId()+ " to stop");
			stop = true;
		}
		
		terminateReader();
	}
	
	private void terminateReader() {
		if (flow.isAllocated()){
			try{
				rina.getIpcManager().requestFlowDeallocation(flow.getPortId());
			}catch(FlowDeallocationException ex){
				ex.printStackTrace();
			}
		}
		
		timer.cancel();
	}
	
	public synchronized boolean isStopped(){
		return stop;
	}

	@Override
	public void dispatchFlowDeallocatedEvent(FlowDeallocatedEvent event) {
		if (flow.getPortId() == event.getPortId()){
			log.info("The flow "+flow.getPortId()+
					" has been deallocated, stopping the fow reader");
		}
		
		stop();
	}
	
}
