package rina.ipcprocess.impl.ribdaemon;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import eu.irati.librina.Flow;

/**
 * Reads sdus from a flow, and 
 * pass them to the RIB Daemon
 * @author eduardgrasa
 */
public class FlowReader implements Runnable{
	
	private Flow flow = null;
	private RIBDaemonImpl ribDaemon = null;
	private int maxSDUSize = 0;
	private boolean stop = false;
	private static final Log log = LogFactory.getLog(FlowReader.class);
	
	public FlowReader(Flow flow, RIBDaemonImpl ribDaemon, int maxSDUSize){
		this.flow = flow;
		this.ribDaemon = ribDaemon;
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
				ribDaemon.cdapMessageDelivered(sdu, flow.getPortId());
			}catch(Exception ex){
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
