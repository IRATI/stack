package rina.ipcprocess.impl.enrollment.ribobjects;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import rina.cdap.api.CDAPSessionDescriptor;
import rina.cdap.api.CDAPSessionManager;
import rina.cdap.api.message.CDAPMessage;
import rina.ipcprocess.impl.IPCProcess;
import rina.ipcprocess.impl.IPCProcess.State;
import rina.ipcprocess.impl.enrollment.EnrollmentTaskImpl;
import rina.ipcprocess.impl.enrollment.statemachines.BaseEnrollmentStateMachine;
import rina.ipcprocess.impl.enrollment.statemachines.EnrolleeStateMachine;
import rina.ribdaemon.api.BaseRIBObject;
import rina.ribdaemon.api.ObjectInstanceGenerator;
import rina.ribdaemon.api.RIBDaemonException;
import rina.ribdaemon.api.RIBObjectNames;

/**
 * Handles the operations related to the "daf.management.operationalStatus" objects
 * @author eduardgrasa
 *
 */
public class OperationalStatusRIBObject extends BaseRIBObject{
	
	private static final Log log = LogFactory.getLog(OperationalStatusRIBObject.class);

	private EnrollmentTaskImpl enrollmentTask = null;
	private CDAPSessionManager cdapSessionManager = null;
	private IPCProcess ipcProcess = null;

	public OperationalStatusRIBObject(EnrollmentTaskImpl enrollmentTaskImpl){
		super(RIBObjectNames.OPERATIONAL_STATUS_RIB_OBJECT_CLASS, 
				ObjectInstanceGenerator.getObjectInstance(), RIBObjectNames.OPERATIONAL_STATUS_RIB_OBJECT_NAME);
		this.enrollmentTask = enrollmentTaskImpl;
		this.ipcProcess = IPCProcess.getInstance();
		this.cdapSessionManager = ipcProcess.getCDAPSessionManager();
	}
	
	@Override
	public void start(CDAPMessage cdapMessage, CDAPSessionDescriptor cdapSessionDescriptor){
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
		
		if (ipcProcess.getOperationalState() != State.ASSIGNED_TO_DIF) {
			ipcProcess.setOperationalState(State.ASSIGNED_TO_DIF);
		}
	}
	
	@Override
	public synchronized void start(Object object) throws RIBDaemonException {
		if (ipcProcess.getOperationalState() != State.ASSIGNED_TO_DIF) {
			ipcProcess.setOperationalState(State.ASSIGNED_TO_DIF);
		}
	}
	
	@Override
	public synchronized void stop(Object object) throws RIBDaemonException {
		if (ipcProcess.getOperationalState() != State.ASSIGNED_TO_DIF) {
			ipcProcess.setOperationalState(State.INITIALIZED);
		}
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
	public synchronized Object getObjectValue(){
		return ipcProcess.getOperationalState();
	}

}
