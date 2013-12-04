package rina.utils.apps.echo.client;

import java.util.Calendar;
import java.util.Timer;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import rina.utils.apps.echo.Echo;
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
	
	public static final long TIMER_PERIOD_IN_MS = 1000;
	
	private Flow flow = null;
	private TestInformation testInformation = null;
	private boolean stop;
	private Timer timer = null;
	private long latestSDUReceivedTime = 0;
	
	private static final Log log = LogFactory.getLog(FlowReader.class);
	
	public FlowReader(Flow flow, TestInformation testInformation, Timer timer){
		this.flow = flow;
		this.testInformation = testInformation;
		this.timer = timer;
	}
	
	private synchronized void setLatestSDUReceivedTime(long time){
		this.latestSDUReceivedTime = time;
	}
	
	private synchronized long getLatestSDUReceivedTime(){
		return latestSDUReceivedTime;
	}
	
	public boolean shouldStop(){
		if (getLatestSDUReceivedTime() + Echo.MAX_TIME_WITH_NO_DATA_IN_MS < 
				Calendar.getInstance().getTimeInMillis()) {
			return true;
		}
		
		return false;
	}

	@Override
	public void run() {
		byte[] buffer = new byte[EchoClient.MAX_SDU_SIZE];
		
		TestDeclaredDeadTimerTask timerTask = new TestDeclaredDeadTimerTask(this, timer);
		timer.schedule(timerTask, TIMER_PERIOD_IN_MS);
		setLatestSDUReceivedTime(Calendar.getInstance().getTimeInMillis());
		
		while(!isStopped()){
			try{
				flow.readSDU(buffer, buffer.length);
				setLatestSDUReceivedTime(Calendar.getInstance().getTimeInMillis());
				testInformation.sduReceived();
				if (testInformation.receivedAllSDUs()) {
					testInformation.setLastSDUReceivedTime(
							Calendar.getInstance().getTimeInMillis());
					printStats();
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
			
			if (!testInformation.receivedAllSDUs()) {
				log.info("Cancelling test since more than " + Echo.MAX_TIME_WITH_NO_DATA_IN_MS + 
						" ms have gone bye without receiving an SDU");
				printStats();
			}
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
	
	private void printStats() {
		if (!testInformation.receivedAllSDUs()) {
			log.info("Received "+testInformation.getSDUsReceived() + " out of " 
					+ testInformation.getNumberOfSDUs() + " SDUs. ");
			testInformation.setLastSDUReceivedTime(getLatestSDUReceivedTime());
		}
		
		long testDuration = testInformation.getLastSDUReceivedTime() 
				- testInformation.getFirstSDUSentTime();
		if (testDuration == 0) {
			testDuration = 1;
		}
		
		log.info("Test completed, sent "+testInformation.getSdusSent() +" and received " + 
				testInformation.getSDUsReceived() + " SDUs of " 
				+ testInformation.getSduSize() + 
				" bytes in " +testDuration + " ms.");
		
		long bandwidthInBps = 1000*testInformation.getNumberOfSDUs()*testInformation.getSduSize()/testDuration;
		log.info("Send and received at " + bandwidthInBps 
				+ " Bytes per second ( " +bandwidthInBps*8/1024 + " Kbps)");
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
