package rina.utils.apps.rinaband.utils;

import eu.irati.librina.Flow;

/**
 * Reads sdus from a flow
 * @author eduardgrasa
 */
public class FlowReader implements Runnable{
	
	private Flow flow;
	private SDUListener sduListener;
	private int maxSDUSize;
	private boolean stop;
	
	public FlowReader(Flow flow, SDUListener sduListener, int maxSDUSize){
		this.flow = flow;
		this.sduListener = sduListener;
		this.maxSDUSize = maxSDUSize;
		this.stop = false;
	}

	@Override
	public void run() {
		byte[] buffer = new byte[maxSDUSize];
		byte[] sdu = null;
		int bytesRead = 0;
		
		try{
			Thread.sleep(1000);
		}catch(Exception ex){
			ex.printStackTrace();
		}
		
		while(!isStopped()){
			try{
				bytesRead = flow.readSDU(buffer, maxSDUSize);
				sdu = new byte[bytesRead];
				for(int i=0; i<sdu.length; i++){
					sdu[i] = buffer[i];
				}
				sduListener.sduDelivered(sdu);
			}catch(Exception ex){
				System.out.println("Problems reading SDU from flow "+flow.getPortId());
				return;
				/*if (isStopped()){
					return;
				}*/
			}
		}
	}
	
	public synchronized void stop(){
		System.out.println("Requesting reader of flow "+flow.getPortId()+ " to stop");
		stop = true;
	}
	
	public synchronized boolean isStopped(){
		return stop;
	}
	
}
