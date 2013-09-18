package rina.utils.apps.echo.server;

import java.util.TimerTask;

public class CancelTestTimerTask extends TimerTask{
	
	private FlowReader flowReader = null;
	
	public CancelTestTimerTask(FlowReader flowReader){
		this.flowReader = flowReader;
	}

	@Override
	public void run() {
		flowReader.stop();
	}

}
