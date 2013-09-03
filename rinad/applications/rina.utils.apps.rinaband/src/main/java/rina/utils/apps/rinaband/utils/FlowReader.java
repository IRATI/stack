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
	
	public FlowReader(Flow flow, SDUListener sduListener, int maxSDUSize){
		this.flow = flow;
		this.sduListener = sduListener;
	}

	@Override
	public void run() {
		byte[] sdu = new byte[maxSDUSize];
		int bytesRead = 0;
		
		while(flow.isAllocated()){
			try{
				bytesRead = flow.readSDU(sdu, maxSDUSize);
				sduListener.sduDelivered(sdu, bytesRead);
			}catch(Exception ex){
				System.out.println("Problems reading SDU from flow "+flow.getPortId());
				ex.printStackTrace();
				if (!flow.isAllocated()){
					break;
				}
			}
		}
	}
	
}
