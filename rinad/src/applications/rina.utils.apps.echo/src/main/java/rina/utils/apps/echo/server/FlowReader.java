package rina.utils.apps.echo.server;

import java.util.Calendar;
import java.util.Timer;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import eu.irati.librina.Flow;
import eu.irati.librina.FlowDeallocationException;
import eu.irati.librina.rina;

/**
 * Reads sdus from a flow
 * @author eduardgrasa
 */
public class FlowReader implements Runnable {
	
	private static final long MAX_TIME_WITH_NO_DATA_IN_MS = 3*1000;
	public static final long TIMER_PERIOD_IN_MS = 1000;
	private Flow flow;
	private int maxSDUSize;
	private boolean stop;
	private Timer timer = null;
	private long latestSDUReceivedTime = 0;
	
	private static final Log log = LogFactory.getLog(FlowReader.class);
	
	public FlowReader(Flow flow, int maxSDUSize){
		this.flow = flow;
		this.maxSDUSize =  maxSDUSize;
		this.stop = false;
		this.timer = new Timer();
	}
	
	private synchronized void setLatestSDUReceivedTime(long time){
		this.latestSDUReceivedTime = time;
	}
	
	private synchronized long getLatestSDUReceivedTime(){
		return latestSDUReceivedTime;
	}

	@Override
	public void run() {
		byte[] buffer = new byte[maxSDUSize];
		int bytesRead = 0;
		
		CancelTestTimerTask timerTask = new CancelTestTimerTask(this);
		timer.schedule(timerTask, TIMER_PERIOD_IN_MS);
		
		while(!isStopped()){
			try{
				bytesRead = flow.readSDU(buffer, maxSDUSize);
				log.debug("Read SDU of size " + bytesRead 
						+ " from portId "+flow.getPortId());
				setLatestSDUReceivedTime(Calendar.getInstance().getTimeInMillis());
				flow.writeSDU(buffer, bytesRead);
				log.debug("Wrote SDU of size " + bytesRead 
						+ " to portId "+flow.getPortId());
			}catch(Exception ex){
				log.error("Problems reading SDU from flow "+flow.getPortId());
				stop();
			}
		}
		
		terminateReader();
	}
	
	public boolean shouldStop(){
		if (getLatestSDUReceivedTime() + MAX_TIME_WITH_NO_DATA_IN_MS < 
				Calendar.getInstance().getTimeInMillis()) {
			return true;
		}
		
		return false;
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
}
