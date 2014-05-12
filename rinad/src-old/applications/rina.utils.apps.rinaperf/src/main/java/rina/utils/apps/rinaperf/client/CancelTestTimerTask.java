package rina.utils.apps.rinaperf.client;

import java.util.TimerTask;

public class CancelTestTimerTask extends TimerTask {
	
	private RINAPerfClient rinaperfClient = null;
	
	public CancelTestTimerTask(RINAPerfClient rinaperfClient) {
		this.rinaperfClient = rinaperfClient;
	}

	@Override
	public void run() {
		rinaperfClient.cancelTest(-1);
	}
}
