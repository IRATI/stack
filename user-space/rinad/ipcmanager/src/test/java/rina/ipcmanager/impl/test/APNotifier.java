package rina.ipcmanager.impl.test;

import rina.ipcservice.api.APService;
import rina.ipcservice.api.FlowService;

public class APNotifier implements Runnable{
	
	public enum Status {NULL, ALLOCATE_RESPONSE_OK, ALLOCATE_RESPONSE_WRONG, DELIVER_TRANSFER};
	
	private APService apService = null;
	private FlowService flowService = null;
	public Status status = Status.NULL;
	
	public APNotifier(APService apService, FlowService flowService, Status status){
		this.apService = apService;
		this.flowService = flowService;
		this.status = status;
	}

	public void run() {
		switch(status){
		case ALLOCATE_RESPONSE_OK:
			apService.deliverAllocateResponse(flowService.getPortId(), 0, null);
			break;
		case DELIVER_TRANSFER:
			//apService.deliverTransfer(flowService.getPortId(), "In Hertford, Hereford, and Hampshire, hurricanes hardly ever happen".getBytes());
			break;
		default:
			
		}
	}

}
