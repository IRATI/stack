package rina.ipcprocess.impl.enrollment.ribobjects;

import java.util.TimerTask;

/**
 * Sends a CDAP message to check that the CDAP connection 
 * is still working ok
 * @author eduardgrasa
 *
 */
public class WatchdogTimerTask extends TimerTask{
	
	private WatchdogRIBObject watchdogRIBObject = null;
	
	public WatchdogTimerTask(WatchdogRIBObject watchdogRIBObject){
		this.watchdogRIBObject = watchdogRIBObject;
	}

	@Override
	public void run() {
		this.watchdogRIBObject.sendMessages();
	}
}
