package rina.utils.apps.echo.client;

import java.util.TimerTask;

/**
 * If this task expires and no SDU from the server has been received, 
 * the test is declared dead
 * @author eduardgrasa
 *
 */
public class TestDeclaredDeadTimerTask extends TimerTask{
	
	public static final long DEFAULT_DELAY_IN_MS = 20*1000;
	
	private FlowReader flowReader = null;
	
	public TestDeclaredDeadTimerTask(FlowReader flowReader){
		this.flowReader = flowReader;
	}

	@Override
	public void run() {
		this.flowReader.stop();
	}

}
