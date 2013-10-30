package rina.utils.apps.echo.client;

import java.util.Timer;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

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
	
	private Flow flow;
	private int sduSize;
	private int numberOfSDUs;
	private boolean stop;
	private Timer timer = null;
	
	private static final Log log = LogFactory.getLog(FlowReader.class);
	
	public FlowReader(Flow flow, int sduSize, int numberOfSDUs){
		this.flow = flow;
		this.sduSize = sduSize;
		this.numberOfSDUs =  numberOfSDUs;
		this.stop = false;
		this.timer = new Timer();
	}

	@Override
	public void run() {
		byte[] buffer = new byte[sduSize];
		int bytesRead = 0;
		
		TestDeclaredDeadTimerTask timerTask = new TestDeclaredDeadTimerTask(this);
		timer.schedule(timerTask, 20*1000);
		int receivedSDUs = 0;
		
		while(!isStopped() && receivedSDUs < numberOfSDUs){
			try{
				bytesRead = flow.readSDU(buffer, buffer.length);
				log.debug("Read SDU of size " + bytesRead 
						+ " from portId "+flow.getPortId());
				receivedSDUs++;
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
		System.exit(0);
	}
	
	public synchronized void stop(){
		log.info("Requesting reader of flow "+flow.getPortId()+ " to stop");
		stop = true;
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
