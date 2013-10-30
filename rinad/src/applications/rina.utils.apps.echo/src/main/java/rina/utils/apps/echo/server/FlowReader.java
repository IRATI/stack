package rina.utils.apps.echo.server;

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
	
	private Flow flow;
	private int maxSDUSize;
	private boolean stop;
	private Timer timer = null;
	
	private static final Log log = LogFactory.getLog(FlowReader.class);
	
	public FlowReader(Flow flow, int maxSDUSize){
		this.flow = flow;
		this.maxSDUSize =  maxSDUSize;
		this.stop = false;
		this.timer = new Timer();
	}

	@Override
	public void run() {
		byte[] buffer = new byte[maxSDUSize];
		int bytesRead = 0;
		
		CancelTestTimerTask timerTask = new CancelTestTimerTask(this);
		timer.schedule(timerTask, 20*1000);
		
		while(!isStopped()){
			try{
				bytesRead = flow.readSDU(buffer, maxSDUSize);
				log.debug("Read SDU of size " + bytesRead 
						+ " from portId "+flow.getPortId());
				flow.writeSDU(buffer, bytesRead);
				log.debug("Wrote SDU of size " + bytesRead 
						+ " to portId "+flow.getPortId());
			}catch(Exception ex){
				log.error("Problems reading SDU from flow "+flow.getPortId());
				if (isStopped()){
					return;
				}
			}
		}
		
		if (flow.isAllocated()){
			try{
				rina.getIpcManager().requestFlowDeallocation(flow.getPortId());
			}catch(FlowDeallocationException ex){
				ex.printStackTrace();
			}
		}
		
		timer.cancel();
	}
	
	public synchronized void stop(){
		log.info("Requesting reader of flow "+flow.getPortId()+ " to stop");
		stop = true;
	}
	
	public synchronized boolean isStopped(){
		return stop;
	}
}
