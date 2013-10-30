package rina.ipcmanager.impl.console;

import java.util.TimerTask;

import eu.irati.librina.TimerExpiredEvent;

/**
 * Cancel a console command if no response has arrived in X seconds
 */
public class CancelCommandTimerTask extends TimerTask {
	
	public static final int INTERVAL_IN_SECONDS = 5;
	private IPCManagerConsole console = null;
	long pendingRequestId = -1;
	
	public CancelCommandTimerTask(IPCManagerConsole console, long pendingRequestId){
		this.console = console;
		this.pendingRequestId = pendingRequestId;
	}

	@Override
	public void run() {
		TimerExpiredEvent event = new TimerExpiredEvent(pendingRequestId);
		console.responseArrived(event);
	}

}
