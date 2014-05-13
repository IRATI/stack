package rina.ipcprocess.impl.enrollment;

import java.util.List;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import eu.irati.librina.Neighbor;

import rina.configuration.RINAConfiguration;
import rina.enrollment.api.EnrollmentRequest;
import rina.enrollment.api.EnrollmentTask;
import rina.ipcprocess.api.IPCProcess;
import rina.ipcprocess.impl.enrollment.ribobjects.NeighborRIBObject;
import rina.ipcprocess.impl.enrollment.ribobjects.NeighborSetRIBObject;
import rina.ribdaemon.api.RIBObjectNames;

/**
 * This class periodically looks for known neighbors we're currently not enrolled to, and tries 
 * to enroll with them again.
 * @author eduardgrasa
 *
 */
public class NeighborsEnroller implements Runnable{

	private static final Log log = LogFactory.getLog(NeighborsEnroller.class);
	
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
	
	public NeighborsEnroller(IPCProcess ipcProcess, EnrollmentTask enrollmentTask){
		this.enrollmentTask = enrollmentTask;
		this.ipcProcess = ipcProcess;
	}
	
	public void run() {
		while(true){
			this.knownNeighbors = ipcProcess.getNeighbors();
			Neighbor neighbor = null;
			for(int i=0; i<this.knownNeighbors.size(); i++){
				neighbor = this.knownNeighbors.get(i);
				if (enrollmentTask.isEnrolledTo(neighbor.getName().getProcessName())){
					//We're enrolled to this guy, continue
					continue;
				}
				
				if (neighbor.getNumberOfEnrollmentAttempts() < 
						RINAConfiguration.getInstance().getLocalConfiguration().getMaxEnrollmentRetries()) {
					neighbor.setNumberOfEnrollmentAttempts(neighbor.getNumberOfEnrollmentAttempts()+1);
					EnrollmentRequest request = new EnrollmentRequest(this.knownNeighbors.get(i), null);
					enrollmentTask.initiateEnrollment(request);
				} else {
					try {
						ipcProcess.getRIBDaemon().delete(NeighborRIBObject.NEIGHBOR_RIB_OBJECT_CLASS, 
								NeighborSetRIBObject.NEIGHBOR_SET_RIB_OBJECT_NAME + RIBObjectNames.SEPARATOR 
								+ neighbor.getName().getProcessName());
						log.info("Removed neighbor: "+neighbor.getName().toString()+" from neighbors list");
					} catch (Exception ex) {
						log.error("Problems removing neighbor from RIB: "+neighbor.getName().toString());
					}
				}
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
