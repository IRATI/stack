package rina.utils.apps.echo.client;

import java.util.Timer;
import java.util.TimerTask;

/**
 * If this task expires and no SDU from the server has been received, 
 * the test is declared dead
 * @author eduardgrasa
 *
 */
public class TestDeclaredDeadTimerTask extends TimerTask{
	
	private FlowReader flowReader = null;
	private Timer timer = null;
	
	public TestDeclaredDeadTimerTask(FlowReader flowReader, Timer timer){
		this.flowReader = flowReader;
		this.timer = timer;
	}

	@Override
	public void run() {
		if (this.flowReader.shouldStop()) {
			this.flowReader.stop();
		} else {
			timer.schedule(
					new TestDeclaredDeadTimerTask(flowReader, timer), 
					FlowReader.TIMER_PERIOD_IN_MS);
		}

	}

}
