package rina.ipcprocess.impl.enrollment.ribobjects;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import rina.cdap.api.CDAPSessionDescriptor;
import rina.cdap.api.CDAPSessionManager;
import rina.cdap.api.message.CDAPMessage;
import rina.enrollment.api.EnrollmentInformationRequest;
import rina.ipcprocess.api.IPCProcess;
import rina.ipcprocess.impl.enrollment.EnrollmentTaskImpl;
import rina.ipcprocess.impl.enrollment.statemachines.BaseEnrollmentStateMachine;
import rina.ipcprocess.impl.enrollment.statemachines.EnrolleeStateMachine;
import rina.ipcprocess.impl.enrollment.statemachines.EnrollerStateMachine;
import rina.ribdaemon.api.BaseRIBObject;
import rina.ribdaemon.api.ObjectInstanceGenerator;
import rina.ribdaemon.api.RIBDaemonException;

/**
 * Handles the operations related to the "daf.management.enrollment" objects
 * @author eduardgrasa
 *
 */
public class EnrollmentRIBObject extends BaseRIBObject{
	
	private static final Log log = LogFactory.getLog(EnrollmentRIBObject.class);
	
	private EnrollmentTaskImpl enrollmentTask = null;
	private CDAPSessionManager cdapSessionManager = null;
	
	public EnrollmentRIBObject(IPCProcess ipcProcess, EnrollmentTaskImpl enrollmentTaskImpl){
		super(ipcProcess, EnrollmentInformationRequest.ENROLLMENT_INFO_OBJECT_CLASS, 
				ObjectInstanceGenerator.getObjectInstance(), EnrollmentInformationRequest.ENROLLMENT_INFO_OBJECT_NAME);
		this.enrollmentTask = enrollmentTaskImpl;
		this.cdapSessionManager = ipcProcess.getCDAPSessionManager();
		setRIBDaemon(ipcProcess.getRIBDaemon());
	}
	
	/**
	 * Called when the IPC Process has received the M_START enrollment message received
	 */
	@Override
	public void start(CDAPMessage cdapMessage, CDAPSessionDescriptor cdapSessionDescriptor) throws RIBDaemonException{
		EnrollerStateMachine enrollmentStateMachine = null;
		
		try{
			enrollmentStateMachine = (EnrollerStateMachine) this.getEnrollmentStateMachine(cdapSessionDescriptor);
		}catch(Exception ex){
			log.error(ex);
			sendErrorMessage(cdapSessionDescriptor);
			return;
		}	
		
		if (enrollmentStateMachine == null){
			log.error("Got a CDAP message that is not for me: "+cdapMessage.toString());
			return;
		}
		
		enrollmentStateMachine.start(cdapMessage, cdapSessionDescriptor);
	}
	
	@Override
	/**
	 * Called when the IPC Process has received the M_START enrollment message received
	 */
	public void stop(CDAPMessage cdapMessage, CDAPSessionDescriptor cdapSessionDescriptor) throws RIBDaemonException{
		EnrolleeStateMachine enrollmentStateMachine = null;
		
		try{
			enrollmentStateMachine = (EnrolleeStateMachine) this.getEnrollmentStateMachine(cdapSessionDescriptor);
		}catch(Exception ex){
			log.error(ex);
			sendErrorMessage(cdapSessionDescriptor);
		}
		
		if (enrollmentStateMachine == null){
			log.error("Got a CDAP message that is not for me: "+cdapMessage.toString());
			return;
		}
		
		enrollmentStateMachine.stop(cdapMessage, cdapSessionDescriptor);
	}
	
	private BaseEnrollmentStateMachine getEnrollmentStateMachine(CDAPSessionDescriptor cdapSessionDescriptor){
		BaseEnrollmentStateMachine enrollmentStateMachine = enrollmentTask.getEnrollmentStateMachine(
				cdapSessionDescriptor.getDestinationApplicationProcessNamingInfo().getProcessName(), 
				cdapSessionDescriptor.getPortId(), false);
		return enrollmentStateMachine;
	}
	
	private void sendErrorMessage(CDAPSessionDescriptor cdapSessionDescriptor){
		try{
			enrollmentTask.getRIBDaemon().sendMessage(cdapSessionManager.getReleaseConnectionRequestMessage(cdapSessionDescriptor.getPortId(), null, false), 
					cdapSessionDescriptor.getPortId(), null);
		}catch(Exception e){
			log.error(e);
		}
	}
	
	@Override
	public Object getObjectValue(){
		return null;
	}
}
