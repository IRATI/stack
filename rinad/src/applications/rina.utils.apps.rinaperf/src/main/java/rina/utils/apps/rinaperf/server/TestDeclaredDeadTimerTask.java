package rina.utils.apps.rinaperf.server;

import java.util.TimerTask;

/**
 * If this task expires and no SDU from the server has been received, 
 * the test is declared dead
 * @author eduardgrasa
 *
 */
public class TestDeclaredDeadTimerTask extends TimerTask{
	
	private TestController flowReader = null;
	
	public TestDeclaredDeadTimerTask(TestController flowReader){
		this.flowReader = flowReader;
	}

	@Override
	public void run() {
		flowReader.cancel();
	}

}
