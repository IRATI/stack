package rina.ipcprocess.impl.enrollment;

import java.util.List;

import eu.irati.librina.Neighbor;

import rina.enrollment.api.EnrollmentTask;
import rina.ipcprocess.impl.IPCProcess;

/**
 * This class periodically looks for known neighbors we're currently not enrolled to, and tries 
 * to enroll with them again.
 * @author eduardgrasa
 *
 */
public class NeighborsEnroller implements Runnable{
	
	public static final long DEFAULT_NEIGHBORS_ENROLLER_PERIOD_IN_MS = 60000;

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
				Thread.sleep(DEFAULT_NEIGHBORS_ENROLLER_PERIOD_IN_MS);
			}catch(Exception ex){
				ex.printStackTrace();
			}
		}
	}

}
