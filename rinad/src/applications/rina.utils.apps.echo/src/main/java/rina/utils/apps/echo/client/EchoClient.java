package rina.utils.apps.echo.client;

import java.util.Calendar;
import java.util.Timer;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import rina.aux.LogHelper;
import rina.cdap.api.CDAPSessionManager;
import rina.cdap.api.message.CDAPMessage;
import rina.cdap.api.message.CDAPMessage.Opcode;
import rina.cdap.api.message.ObjectValue;
import rina.cdap.impl.CDAPSessionManagerImpl;
import rina.cdap.impl.googleprotobuf.GoogleProtocolBufWireMessageProviderFactory;
import rina.utils.apps.echo.TestInformation;
import rina.utils.apps.echo.protobuf.EchoTestMessageEncoder;
import rina.utils.apps.echo.utils.ApplicationRegistrationListener;
import rina.utils.apps.echo.utils.FlowAllocationListener;
import rina.utils.apps.echo.utils.FlowDeallocationListener;
import rina.utils.apps.echo.utils.IPCEventConsumer;
import eu.irati.librina.AllocateFlowRequestResultEvent;
import eu.irati.librina.ApplicationProcessNamingInformation;
import eu.irati.librina.ApplicationRegistrationInformation;
import eu.irati.librina.ApplicationRegistrationType;
import eu.irati.librina.Flow;
import eu.irati.librina.FlowDeallocatedEvent;
import eu.irati.librina.FlowSpecification;
import eu.irati.librina.RegisterApplicationResponseEvent;
import eu.irati.librina.UnregisterApplicationResponseEvent;
import eu.irati.librina.rina;

/**
 * Implements the behavior of a RINABand Client
 * @author eduardgrasa
 *
 */
public class EchoClient implements  ApplicationRegistrationListener, 
FlowAllocationListener, FlowDeallocationListener {
	
	public static final int MAX_SDU_SIZE = 10000;
	public static final int MAX_NUM_OF_SDUS = 10000;
	public static final String TEST_OBJECT_CLASS = "Echo test";
	public static final String TEST_OBJECT_NAME = "/rina/utils/apps/echo/test";
	
	private TestInformation testInformation = null;
	
	private static final Log log = LogFactory.getLog(EchoClient.class);
	
	/**
	 * The APNamingInfo associated to the control AE of the Echo application
	 */
	private ApplicationProcessNamingInformation echoApNamingInfo = null;
	/**
	 * The client AP Naming Information
	 */
	private ApplicationProcessNamingInformation clientApNamingInfo = null;
	
	/**
	 * The flow to the Echo server AE
	 */
	private Flow flow = null;
	
	/**
	 * The thread pool
	 */
	private ExecutorService executorService = null;
	
	private IPCEventConsumer ipcEventConsumer = null;
	private long handle = -1;
	private CDAPSessionManager cdapSessionManager = null;
	
	private FlowReader flowReader = null;
	
	private Timer timer = null;
	
	public EchoClient(int numberOfSdus, int sduSize, 
			ApplicationProcessNamingInformation echoApNamingInfo, 
			ApplicationProcessNamingInformation clientApNamingInfo){
		try {
			rina.initialize(LogHelper.getLibrinaLogLevel());
		} catch(Exception ex){
			log.error("Problems initializing librina, exiting: "+ex.getMessage());
			System.exit(-1);
		}
		
		testInformation = new TestInformation();
		if (numberOfSdus > MAX_NUM_OF_SDUS) {
			numberOfSdus = MAX_NUM_OF_SDUS;
		}
		testInformation.setNumberOfSDUs(numberOfSdus);
		
		if (sduSize > MAX_SDU_SIZE) {
			sduSize = MAX_SDU_SIZE;
		}
		testInformation.setSduSize(sduSize);
		
		this.echoApNamingInfo = echoApNamingInfo;
		this.clientApNamingInfo = clientApNamingInfo;
		
		cdapSessionManager = new CDAPSessionManagerImpl(
				new GoogleProtocolBufWireMessageProviderFactory());
		
		timer = new Timer();
		
		ipcEventConsumer = new IPCEventConsumer();
		ipcEventConsumer.addApplicationRegistrationListener(this, clientApNamingInfo);
		executorService = Executors.newCachedThreadPool();
		executorService.execute(ipcEventConsumer);
	}
	
	public void execute(){
		//0 Register client application (otherwise we cannot use the shim DIF)
		try{
			ApplicationRegistrationInformation appRegInfo = 
					new ApplicationRegistrationInformation(
							ApplicationRegistrationType.APPLICATION_REGISTRATION_ANY_DIF);
			appRegInfo.setApplicationName(clientApNamingInfo);
			rina.getIpcManager().requestApplicationRegistration(appRegInfo);
			log.info("Requested registration of AE: "+clientApNamingInfo.toString());
		}catch(Exception ex){
			ex.printStackTrace();
			System.exit(-1);
		}
	}
	
	public void cancelTest(int status) {
		if (flowReader != null) {
			flowReader.stop();
		}
		
		System.exit(status);
	}
	
	public synchronized void dispatchApplicationRegistrationResponseEvent(
			RegisterApplicationResponseEvent event) {
		if (event.getResult() == 0) {
			try {
				rina.getIpcManager().commitPendingResitration(
						event.getSequenceNumber(), event.getDIFName());
				log.info("Succesfully registered AE " + event.getApplicationName().toString() 
						+ " to DIF" + event.getDIFName().getProcessName());
				
				//1 Allocate a flow to the Echo Server AE
				FlowSpecification qosSpec = new FlowSpecification();
				handle = rina.getIpcManager().requestFlowAllocation(
						this.clientApNamingInfo, this.echoApNamingInfo, qosSpec);
				ipcEventConsumer.addFlowAllocationListener(this, handle);
			} catch (Exception ex){
				log.error(ex.getMessage());
				System.exit(-1);
			}	
		} else {
			try{
				log.error("Problems registering AE "+ event.getApplicationName().toString() 
						+ ". Error code: " + event.getResult());
				rina.getIpcManager().withdrawPendingRegistration(event.getSequenceNumber());
			}catch(Exception ex) {
				log.error(ex.getMessage());
			}
			System.exit(event.getResult());
		}
	}
	
	public synchronized void dispatchApplicationUnregistrationResponseEvent(
			UnregisterApplicationResponseEvent event) {
		boolean success = false;
		
		if (event.getResult() == 0){
			success = true;
		}
		
		try {
			rina.getIpcManager().appUnregistrationResult(event.getSequenceNumber(), success);
		} catch (Exception ex) {
			log.error(ex.getMessage());
		}
	}
	
	@Override
	public synchronized void dispatchAllocateFlowRequestResultEvent(
			AllocateFlowRequestResultEvent event) {
		ipcEventConsumer.removeFlowAllocationListener(event.getSequenceNumber());

		if (event.getSequenceNumber() == handle){
			if (event.getPortId() > 0){
				try{
					flow = rina.getIpcManager().commitPendingFlow(event.getSequenceNumber(), 
							event.getPortId(), event.getDIFName());
				}catch(Exception ex){
					log.error(ex.getMessage());
					System.exit(-1);
				}
				
				ipcEventConsumer.addFlowDeallocationListener(this, event.getPortId());
				
				byte[] buffer = null;
				int bytesRead = 0;
				try{
					ObjectValue objectValue = new ObjectValue();
					objectValue.setByteval(EchoTestMessageEncoder.encode(this.testInformation));
					CDAPMessage cdapMessage = CDAPMessage.getStartObjectRequestMessage(
							null, null, TEST_OBJECT_CLASS, objectValue, 0, TEST_OBJECT_NAME, 0);
					cdapMessage.setInvokeID(1);
					buffer = this.cdapSessionManager.encodeCDAPMessage(cdapMessage);
					flow.writeSDU(buffer, buffer.length);
					log.info("Requested echo server to start a test with the following parameters: \n" 
							+ testInformation.toString());
					
					CancelTestTimerTask timerTask = new CancelTestTimerTask(this);
					timer.schedule(timerTask, 2*1000);
					
					buffer = new byte[MAX_SDU_SIZE];
					bytesRead = flow.readSDU(buffer, buffer.length);
					timerTask.cancel();
					byte[] sdu = new byte[bytesRead];
					for(int i=0; i<sdu.length; i++) {
						sdu[i] = buffer[i];
					}
					cdapMessage = cdapSessionManager.decodeCDAPMessage(sdu);
					if (cdapMessage.getOpCode() != Opcode.M_START_R) {
						throw new Exception("Received wrong CDAP message code: "+cdapMessage.getOpCode());
					} else if (cdapMessage.getResult() != 0) {
						throw new Exception("Echo server rejected the test");
					}
					
					log.info("Echo server accepted the test, starting...");
				} catch(Exception ex) {
					log.error("Error initiating test: "+ex.getMessage() 
							+ ". Deallocating flow and terminating");
					try {
						rina.getIpcManager().requestFlowDeallocation(flow.getPortId());
					} catch(Exception e) {
						log.error("Problems requesting flow deallocation " +e.getMessage());
						System.exit(-1);
					}
					return;
				}
				
				//2 Start flowReader
				flowReader = new FlowReader(flow, this.testInformation, timer);
				this.executorService.execute(flowReader);

				//3 Send SDUs to server
				buffer = new byte[testInformation.getSduSize()];
				for(int i=0; i<testInformation.getNumberOfSDUs(); i++){
					try{
						flow.writeSDU(buffer, buffer.length);
						if (i==0) {
							testInformation.setFirstSDUSentTime(
									Calendar.getInstance().getTimeInMillis());
						}
					}catch(Exception ex){
						log.error("Error writing SDU "+i+" to port-id "+flow.getPortId());
					}
				}
			} else {
				log.error("Problems allocating flow to control AE: " + echoApNamingInfo.toString());
				
				try{
					rina.getIpcManager().withdrawPendingFlow(event.getSequenceNumber());
				} catch (Exception ex) {
					log.error(ex.getMessage());
				}
				
				System.exit(-1);
			}
		}
	}
	
	@Override
	public void dispatchFlowDeallocatedEvent(FlowDeallocatedEvent event) {
		if (flowReader != null) {
			flowReader.stop();
		}
		
		System.exit(0);
	}
}
