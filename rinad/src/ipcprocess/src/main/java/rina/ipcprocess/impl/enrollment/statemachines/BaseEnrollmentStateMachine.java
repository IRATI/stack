package rina.ipcprocess.impl.enrollment.statemachines;

import java.util.Timer;
import java.util.TimerTask;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import eu.irati.librina.ApplicationProcessNamingInformation;
import eu.irati.librina.Neighbor;

import rina.cdap.api.BaseCDAPMessageHandler;
import rina.cdap.api.CDAPException;
import rina.cdap.api.CDAPSessionDescriptor;
import rina.cdap.api.CDAPSessionManager;
import rina.cdap.api.message.CDAPMessage;
import rina.encoding.api.Encoder;
import rina.enrollment.api.EnrollmentTask;
import rina.ipcprocess.impl.enrollment.ribobjects.NeighborRIBObject;
import rina.ipcprocess.impl.enrollment.ribobjects.NeighborSetRIBObject;
import rina.ribdaemon.api.RIBDaemon;
import rina.ribdaemon.api.RIBObjectNames;

/**
 * The base class that contains the common aspects of both 
 * enrollment state machines: the enroller side and the enrolle side
 * @author eduardgrasa
 *
 */
public abstract class BaseEnrollmentStateMachine extends BaseCDAPMessageHandler{
	
	private static final Log log = LogFactory.getLog(BaseEnrollmentStateMachine.class);
	
	public static final String CONNECT_IN_NOT_NULL = "Received a CONNECT message while not in NULL state";
	public static final String CONNECT_RESPONSE_TIMEOUT = "Timeout waiting for connect response";
	public static final String CREATE_IN_BAD_STATE = "Received a CREATE message in a wrong state";
	public static final String PROBLEMS_COMITING_ENROLLMENT_INFO = "Problems comiting enrollment information";
	public static final String READ_RESPONSE_IN_BAD_STATE = "Received a READ response in a wrong state";
	public static final String READ_RESPONSE_TIMEOUT = "Timeout waiting for read response";
	public static final String START_ENROLLMENT_TIMEOUT = "Timeout waiting for start enrollment request";
	public static final String START_IN_BAD_STATE = "Received a START message in a wrong state";
	public static final String START_RESPONSE_IN_BAD_STATE = "Received a START response in a wrong state";
	public static final String START_RESPONSE_TIMEOUT = "Timeout waiting for start response";
	public static final String START_TIMEOUT = "Timeout waiting for start";
	public static final String STOP_ENROLLMENT_RESPONSE_TIMEOUT = "Timeout waiting for stop enrolment response";
	public static final String STOP_IN_BAD_STATE = "Received a STOP message in a wrong state";
	public static final String STOP_RESPONSE_IN_BAD_STATE = "Received a STOP response in a wrong state";
	public static final String STOP_ENROLLMENT_TIMEOUT = "Timeout waiting for stop enrollment request";
	public static final String STOP_WITH_NO_OBJECT_VALUE = "Received STOP message with null object value";
	public static final String UNEXPECTED_ERROR = "Unexpected error. ";
	public static final String UNSUCCESSFULL_READ_RESPONSE = "Received an unsuccessful read response or a read response with a null object value";
	public static final String UNSUCCESSFULL_START = "Received unsuccessful start request";
	
	/**
	 * All the possible states of all the enroller state machines
	 */
	public enum State {NULL, WAIT_CONNECT_RESPONSE, WAIT_START_ENROLLMENT_RESPONSE, WAIT_READ_RESPONSE, 
		WAIT_START, ENROLLED, WAIT_START_ENROLLMENT, WAIT_STOP_ENROLLMENT_RESPONSE};
		
	/**
	 * the current state
	 */
	protected State state = State.NULL;

	/**
	 * The RMT to post the return messages
	 */
	protected RIBDaemon ribDaemon = null;

	/**
	 * The CDAPSessionManager, to encode/decode cdap messages
	 */
	protected CDAPSessionManager cdapSessionManager = null;

	/**
	 * The encoded to encode/decode the object values in CDAP messages
	 */
	protected Encoder encoder = null;
	
	/**
	 * The timer that will execute the different timer tasks of this class
	 */
	protected Timer timer = null;

	/**
	 * The timer task used by the timer
	 */
	protected TimerTask timerTask = null;

	/**
	 * The portId to use
	 */
	protected int portId = 0;
	
	/**
	 * The enrollment task
	 */
	protected EnrollmentTask enrollmentTask = null;
	
	/**
	 * The maximum time to wait between steps of the enrollment sequence (in ms)
	 */
	protected long timeout = 0;
	
	/**
	 * The information of the remote IPC Process being enrolled
	 */
	protected Neighbor remotePeer = null;
	
	protected BaseEnrollmentStateMachine(RIBDaemon ribDaemon, CDAPSessionManager cdapSessionManager, Encoder encoder, 
			ApplicationProcessNamingInformation remoteNamingInfo, EnrollmentTask enrollmentTask, long timeout){
		this.ribDaemon = ribDaemon;
		this.cdapSessionManager = cdapSessionManager;
		this.encoder = encoder;
		this.enrollmentTask = enrollmentTask;
		remotePeer = new Neighbor();
		remotePeer.setName(remoteNamingInfo);
		this.timeout = timeout;
		timer = new Timer();
	}
	
	public State getState(){
		return this.state;
	}
	
	protected synchronized void setState(State state){
		this.state = state;
	}
	
	public int getPortId(){
		return this.portId;
	}
	
	public ApplicationProcessNamingInformation getRemotePeerNamingInfo(){
		return remotePeer.getName();
	}
	
	/**
	 * Send a CDAP message using the RMT
	 * @param cdapMessage
	 * @param portId
	 */
	protected void sendCDAPMessage(CDAPMessage cdapMessage){
		try{
			ribDaemon.sendMessage(cdapMessage, portId, this);
		}catch(Exception ex){
			ex.printStackTrace();
			log.error("Could not send CDAP message: "+ex.getMessage());
			if (ex.getMessage().equals("Flow closed")){
				cdapSessionManager.removeCDAPSession(portId);
			}
		}
	}

	/**
	 * Returns a timer task that will disconnect the CDAP session when 
	 * the timer task runs.
	 * @return
	 */
	protected TimerTask getEnrollmentFailedTimerTask(String reason, boolean enrollee){
		final String message = reason;
		final boolean isEnrollee = enrollee;
		return new TimerTask(){
			@Override
			public void run() {
				try{
					abortEnrollment(remotePeer.getName(), portId, message, isEnrollee, true);
				}catch(Exception ex){
					ex.printStackTrace();
				}
			}};
	}
	
	protected boolean isValidPortId(CDAPSessionDescriptor cdapSessionDescriptor){
		if (cdapSessionDescriptor.getPortId() != this.getPortId()){
			log.error("Received a CDAP Message from port id "+
					cdapSessionDescriptor.getPortId()+". Was expecting messages from port id "+this.getPortId());
			return false;
		}

		return true;
	}
	
	/**
	 * Called by the enrollment state machine when the enrollment sequence fails
	 * @param remotePeer
	 * @param portId
	 * @param enrollee
	 * @param sendMessage
	 * @param reason
	 */
	protected void abortEnrollment(ApplicationProcessNamingInformation remotePeerNamingInfo, int portId, 
			 String reason, boolean enrollee, boolean sendReleaseMessage){
		synchronized(this){
			timer.cancel();
			setState(State.NULL);
		}
		
		enrollmentTask.enrollmentFailed(remotePeerNamingInfo, portId, reason, enrollee, sendReleaseMessage);
	}
	
	/**
	 * Called by the EnrollmentTask when it got an M_RELEASE message
	 * @param cdapMessage
	 * @param cdapSessionDescriptor
	 */
	public void release(CDAPMessage cdapMessage, CDAPSessionDescriptor cdapSessionDescriptor){
		log.debug("Releasing the CDAP connection");
		if (!isValidPortId(cdapSessionDescriptor)){
			return;
		}
		
		createOrUpdateNeighborInformation(false);

		synchronized(this){
			this.setState(State.NULL);

			//Cancel any timers
			if (timer != null){
				timer.cancel();
			}
		}

		if (cdapMessage.getInvokeID() != 0){
			try{
				sendCDAPMessage(cdapSessionManager.getReleaseConnectionResponseMessage(portId, null, 0, null, cdapMessage.getInvokeID()));
			}catch(CDAPException ex){
				log.error(ex);
			}
		}
	}
	
	/**
	 * Called by the EnrollmentTask when it got an M_RELEASE_R message
	 * @param cdapMessage
	 * @param cdapSessionDescriptor
	 */
	public void releaseResponse(CDAPMessage cdapMessage, CDAPSessionDescriptor cdapSessionDescriptor){
		if (!isValidPortId(cdapSessionDescriptor)){
			return;
		}

		synchronized(this){
			if (!this.getState().equals(State.NULL)){
				this.setState(State.NULL);
			}
		}
	}
	
	/**
	 * Called by the EnrollmentTask when the flow supporting the CDAP session with the remote peer
	 * has been deallocated
	 * @param cdapSessionDescriptor
	 */
	public void flowDeallocated(CDAPSessionDescriptor cdapSessionDescriptor){
		log.info("The flow supporting the CDAP session identified by "+cdapSessionDescriptor.getPortId()
				+" has been deallocated.");

		if (!isValidPortId(cdapSessionDescriptor)){
			return;
		}
		
		createOrUpdateNeighborInformation(false);
		
		synchronized(this){
			this.setState(State.NULL);

			//Cancel any timers
			if (timer != null){
				timer.cancel();
			}
		}
	}
	
	/**
	 * Create or update the neighbor information in the RIB
	 * @param enrolled true if the neighbor is enrolled, false otherwise
	 */
	protected void createOrUpdateNeighborInformation(boolean enrolled){
		//Create or update the neighbor information in the RIB
		try{
			remotePeer.setEnrolled(enrolled);
			remotePeer.setNumberOfEnrollmentAttempts(0);
			if (enrolled){
				remotePeer.setUnderlyingPortId(this.getPortId());
			}else{
				remotePeer.setUnderlyingPortId(0);
			}
			
			ribDaemon.create(NeighborRIBObject.NEIGHBOR_RIB_OBJECT_CLASS, 
					NeighborSetRIBObject.NEIGHBOR_SET_RIB_OBJECT_NAME + RIBObjectNames.SEPARATOR 
						+ remotePeer.getName().getProcessName(), 
					remotePeer);
		}catch(Exception ex){
			ex.printStackTrace();
		}
	}
}
