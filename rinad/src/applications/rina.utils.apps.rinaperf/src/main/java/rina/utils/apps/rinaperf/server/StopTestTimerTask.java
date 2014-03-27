package rina.utils.apps.rinaperf.server;

import java.util.TimerTask;

public class StopTestTimerTask extends TimerTask{
	
	private TestController flowReader = null;
	
	public StopTestTimerTask(TestController flowReader){
		this.flowReader = flowReader;
	}

	@Override
	public void run() {
		flowReader.stop();
	}

}
