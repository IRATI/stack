package rina.utils.apps.echo.server;

import java.util.TimerTask;

public class CancelTestTimerTask extends TimerTask{
	
	private TestController flowReader = null;
	
	public CancelTestTimerTask(TestController flowReader){
		this.flowReader = flowReader;
	}

	@Override
	public void run() {
		flowReader.stop();
	}

}
