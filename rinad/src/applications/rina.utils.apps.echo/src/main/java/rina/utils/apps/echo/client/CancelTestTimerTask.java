package rina.utils.apps.echo.client;

import java.util.TimerTask;

public class CancelTestTimerTask extends TimerTask {
	
	private EchoClient echoClient = null;
	
	public CancelTestTimerTask(EchoClient echoClient) {
		this.echoClient = echoClient;
	}

	@Override
	public void run() {
		echoClient.cancelTest(-1);
	}
}
