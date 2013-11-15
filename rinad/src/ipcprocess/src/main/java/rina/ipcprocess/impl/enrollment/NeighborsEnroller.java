package rina.ipcprocess.impl.enrollment;

import java.util.List;

import eu.irati.librina.Neighbor;

import rina.configuration.RINAConfiguration;
import rina.enrollment.api.EnrollmentTask;
import rina.ipcprocess.impl.IPCProcess;

/**
 * This class periodically looks for known neighbors we're currently not enrolled to, and tries 
 * to enroll with them again.
 * @author eduardgrasa
 *
 */
public class NeighborsEnroller implements Runnable{

	/**
	 * The list of known neighbors
	 */
	private List<Neighbor> knownNeighbors = null;
	
	/**
	 * The enrollment task
	 */
	private EnrollmentTask enrollmentTask = null;
	
	/**
	 * The IPC Process
	 */
	private IPCProcess ipcProcess = null;
	
	public NeighborsEnroller(EnrollmentTask enrollmentTask){
		this.enrollmentTask = enrollmentTask;
		ipcProcess = IPCProcess.getInstance();
	}
	
	public void run() {
		while(true){
			this.knownNeighbors = ipcProcess.getNeighbors();
			for(int i=0; i<this.knownNeighbors.size(); i++){
				if (enrollmentTask.isEnrolledTo(
						this.knownNeighbors.get(i).getName().getProcessName())){
					//We're enrolled to this guy, continue
					continue;
				}
				
				enrollmentTask.initiateEnrollment(this.knownNeighbors.get(i));
			}
			
			try{
				Thread.sleep(
						RINAConfiguration.getInstance().getLocalConfiguration().getNeighborsEnrollerPeriodInMs());
			}catch(Exception ex){
				ex.printStackTrace();
			}
		}
	}

}
