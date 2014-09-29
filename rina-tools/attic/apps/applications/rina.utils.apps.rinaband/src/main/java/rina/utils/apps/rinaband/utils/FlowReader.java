package rina.utils.apps.rinaband.utils;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

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
	private static final Log log = LogFactory.getLog(FlowReader.class);
	
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
		
		log.info("Starting reader of flow "+flow.getPortId());
		
		while(!isStopped()){
			try{
				bytesRead = flow.readSDU(buffer, maxSDUSize);
				sdu = new byte[bytesRead];
				for(int i=0; i<sdu.length; i++){
					sdu[i] = buffer[i];
				}
				sduListener.sduDelivered(sdu);
			}catch(eu.irati.librina.Exception ex){
				log.error("Problems reading SDU from flow "+flow.getPortId());
				stop();
			}
		}
	}
	
	public synchronized void stop(){
		if (!stop) {
			log.info("Requesting reader of flow "+flow.getPortId()+ " to stop");
			stop = true;
		}
	}
	
	public synchronized boolean isStopped(){
		return stop;
	}
	
}
