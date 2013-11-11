package rina.utils.apps.echo.server;

import java.util.Timer;
import java.util.TimerTask;

/**
 * If this task expires and no SDU from the server has been received, 
 * the test is declared dead
 * @author eduardgrasa
 *
 */
public class TestDeclaredDeadTimerTask extends TimerTask{
	
	public static final long DEFAULT_DELAY_IN_MS = 20*1000;
	
	private TestController flowReader = null;
	private Timer timer = null;
	
	public TestDeclaredDeadTimerTask(TestController flowReader, Timer timer){
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
					TestController.TIMER_PERIOD_IN_MS);
		}

	}

}
