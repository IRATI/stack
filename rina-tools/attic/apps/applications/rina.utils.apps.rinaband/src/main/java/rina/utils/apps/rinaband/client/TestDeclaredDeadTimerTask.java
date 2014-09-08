package rina.utils.apps.rinaband.client;

import java.util.TimerTask;

/**
 * If this task expires and no SDU from the server has been received, 
 * the test is declared dead
 * @author eduardgrasa
 *
 */
public class TestDeclaredDeadTimerTask extends TimerTask{
	
	public static final long DEFAULT_DELAY_IN_MS = 10*1000;
	
	private TestWorker testWorker = null;
	
	public TestDeclaredDeadTimerTask(TestWorker testWorker){
		this.testWorker = testWorker;
	}

	@Override
	public void run() {
		this.testWorker.notWaitingToReceiveMoreSDUs();
	}

}
