package rina.ipcprocess.impl.flowallocator.timertasks;

import java.util.TimerTask;

import rina.ipcprocess.impl.flowallocator.FlowAllocatorInstanceImpl;

public class TearDownFlowTimerTask extends TimerTask{
	
	public static final int DELAY = 5*1000;
	
	private FlowAllocatorInstanceImpl flowAllocatorInstance = null;
	private String flowObjectName = null;
	private boolean requestor = false;

	public TearDownFlowTimerTask(FlowAllocatorInstanceImpl flowAllocatorInstance, 
			String flowObjectName, boolean requestor){
		this.flowAllocatorInstance = flowAllocatorInstance;
		this.flowObjectName = flowObjectName;
		this.requestor = requestor;
	}

	@Override
	public void run() {
		flowAllocatorInstance.destroyFlowAllocatorInstance(flowObjectName, requestor);
	}
}
