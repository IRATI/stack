package rina.utils.apps.rinaband.server;

import java.util.TimerTask;

public class CancelTestTimerTask extends TimerTask{
	
	private TestController testController = null;
	
	public CancelTestTimerTask(TestController testController){
		this.testController = testController;
	}

	@Override
	public void run() {
		testController.timerExpired();
	}

}
