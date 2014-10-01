package rina.utils.apps.rinaperf.client;

import java.util.TimerTask;

public class StopTestTimerTask extends TimerTask {
	
	private RINAPerfClient rinaperfClient = null;
	
	public StopTestTimerTask(RINAPerfClient rinaperfClient) {
		this.rinaperfClient = rinaperfClient;
	}

	@Override
	public void run() {
		rinaperfClient.stop();
	}
}
