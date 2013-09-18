package rina.utils.apps.echo.server;

import java.util.Timer;

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
	private int maxSDUSize;
	private boolean stop;
	private Timer timer = null;
	
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
				flow.writeSDU(buffer, bytesRead);
			}catch(Exception ex){
				System.out.println("Problems reading SDU from flow "+flow.getPortId());
				if (isStopped()){
					return;
				}
			}
		}
		
		if (flow.isAllocated()){
			try{
				rina.getIpcManager().deallocateFlow(flow.getPortId());
			}catch(FlowDeallocationException ex){
				ex.printStackTrace();
			}
		}
		
		timer.cancel();
	}
	
	public synchronized void stop(){
		System.out.println("Requesting reader of flow "+flow.getPortId()+ " to stop");
		stop = true;
	}
	
	public synchronized boolean isStopped(){
		return stop;
	}

	@Override
	public void dispatchFlowDeallocatedEvent(FlowDeallocatedEvent event) {
		if (flow.getPortId() == event.getPortId()){
			System.out.println("The flow "+flow.getPortId()+
					" has been deallocated, stopping the fow reader");
		}
		
		stop();
	}
	
}
