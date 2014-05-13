package rina.utils.apps.rinaband.server;

import java.util.Hashtable;
import java.util.Map;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import eu.irati.librina.ApplicationProcessNamingInformation;
import eu.irati.librina.ApplicationRegistrationInformation;
import eu.irati.librina.ApplicationRegistrationType;
import eu.irati.librina.Flow;
import eu.irati.librina.FlowDeallocatedEvent;
import eu.irati.librina.FlowRequestEvent;
import eu.irati.librina.IPCManagerSingleton;
import eu.irati.librina.RegisterApplicationResponseEvent;
import eu.irati.librina.UnregisterApplicationResponseEvent;
import eu.irati.librina.rina;

import rina.utils.LogHelper;
import rina.cdap.api.CDAPSessionManager;
import rina.cdap.impl.CDAPSessionManagerImpl;
import rina.cdap.impl.googleprotobuf.GoogleProtocolBufWireMessageProviderFactory;
import rina.utils.apps.rinaband.utils.ApplicationRegistrationListener;
import rina.utils.apps.rinaband.utils.FlowAcceptor;
import rina.utils.apps.rinaband.utils.FlowDeallocationListener;
import rina.utils.apps.rinaband.utils.FlowReader;
import rina.utils.apps.rinaband.utils.IPCEventConsumer;

/**
 * Implements the behavior of a RINABand Server
 * @author eduardgrasa
 *
 */
public class RINABandServer implements FlowAcceptor, FlowDeallocationListener, ApplicationRegistrationListener {
	
	public static final int MAX_CONCURRENT_TESTS = 10;
	public static final int MAX_NUMBER_OF_FLOWS = 100;
	public static final int MAX_SDU_SIZE_IN_BYTES = 10000;
	public static final int MAX_SDUS_PER_FLOW = 1000000;
	
	/**
	 * The APNamingInfo associated to the control AE of the RINABand application
	 */
	private ApplicationProcessNamingInformation controlApNamingInfo = null;
	
	/**
	 * The APNamingInfo associated to the data AE of the RINABand application
	 */
	private ApplicationProcessNamingInformation dataApNamingInfo = null;
	
	/** The name of the DIF where the RINABandServer has registered */
	private ApplicationProcessNamingInformation difName = null;
	
	/**
	 * The control flows from RINABand clients
	 */
	private Map<Integer, TestController> ongoingTests = null;
	
	/**
	 * Manages the CDAP sessions to the control AE
	 */
	private CDAPSessionManager cdapSessionManager = null;
	
	private IPCEventConsumer ipcEventConsumer = null;
	
	private static ExecutorService executorService = Executors.newCachedThreadPool();
	
	private static final Log log = LogFactory.getLog(RINABandServer.class);
	
	public RINABandServer(ApplicationProcessNamingInformation controlApNamingInfo, 
			ApplicationProcessNamingInformation dataApNamingInfo) {
		try {
			rina.initialize(LogHelper.getLibrinaLogLevel(), 
					LogHelper.getLibrinaLogFile());
		} catch(Exception ex){
			log.error("Problems initializing librina, exiting: "+ex.getMessage());
			System.exit(-1);
		}
		
		this.controlApNamingInfo = controlApNamingInfo;
		this.dataApNamingInfo = dataApNamingInfo;
		ongoingTests = new Hashtable<Integer, TestController>();
		cdapSessionManager = new CDAPSessionManagerImpl(new GoogleProtocolBufWireMessageProviderFactory());
		ipcEventConsumer = new IPCEventConsumer();
		ipcEventConsumer.addFlowAcceptor(this, controlApNamingInfo);
		ipcEventConsumer.addApplicationRegistrationListener(this, controlApNamingInfo);
		executeRunnable(ipcEventConsumer);
	}
	
	public synchronized static void executeRunnable(Runnable runnable){
		executorService.execute(runnable);
	}
	
	public void execute(){
		//Register the control AE and wait for new RINABand clients to come
		try{
			ApplicationRegistrationInformation appRegInfo = 
					new ApplicationRegistrationInformation(
							ApplicationRegistrationType.APPLICATION_REGISTRATION_ANY_DIF);
			appRegInfo.setApplicationName(controlApNamingInfo);
			rina.getIpcManager().requestApplicationRegistration(appRegInfo);
			log.info("Requested registration of control AE: "+controlApNamingInfo.toString());
		}catch(Exception ex){
			ex.printStackTrace();
			System.exit(-1);
		}
	}
	
	public synchronized void dispatchApplicationRegistrationResponseEvent(
			RegisterApplicationResponseEvent event) {
		if (event.getResult() == 0) {
			try {
				rina.getIpcManager().commitPendingResitration(
						event.getSequenceNumber(), event.getDIFName());
				difName = event.getDIFName();
				log.info("Succesfully registered control AE " + event.getApplicationName().toString() 
						+ " to DIF" + difName.getProcessName());
			} catch (Exception ex){
				log.error(ex.getMessage());
			}
		} else {
			try{
				log.error("Problems registering control AE "+ event.getApplicationName().toString() 
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

	/**
	 * Called when a new RINABand client allocates a flow to the control AE 
	 * of the RINABand Server, in order to negotiate the new test parameters
	 */
	public synchronized void flowAllocated(Flow flow) {
		TestController testController = new TestController(dataApNamingInfo, difName, flow, 
				this.cdapSessionManager, ipcEventConsumer);
		FlowReader flowReader = new FlowReader(flow, testController, 10000);
		testController.setFlowReader(flowReader);
		RINABandServer.executeRunnable(flowReader);
		ongoingTests.put(new Integer(flow.getPortId()), testController);
		ipcEventConsumer.addFlowDeallocationListener(this, flow.getPortId());
		log.info("New flow to the control AE allocated, with port id "+flow.getPortId());
	}

	/**
	 * Called when the control flow with the RINABand client is deallocated
	 */
	public synchronized void flowDeallocated(int portId) {
		TestController testController = ongoingTests.remove(new Integer(portId));
		testController.getFlowReader().stop();
		ipcEventConsumer.removeFlowDeallocationListener(portId);
		
		try {
			rina.getIpcManager().flowDeallocated(portId);
		} catch (Exception ex) {
			log.warn("Error updating allcocated flows for flow "+portId
					 + ". " + ex.getMessage());
		}
		
		log.info("Control flow with port id "+portId+" deallocated");
	}

	/**
	 * Decide when a flow can be accepted
	 */
	public void dispatchFlowRequestEvent(FlowRequestEvent event){
		IPCManagerSingleton ipcManager = rina.getIpcManager();
		if (this.difName == null) {
			log.error("Cannot process flow requests until the control AE is registered");
			return;
		}
		
		if (this.ongoingTests.size() >= MAX_CONCURRENT_TESTS){
			try{
				ipcManager.allocateFlowResponse(event, -1, true);
			}catch(Exception ex){
				ex.printStackTrace();
			}
			return;
		}

		try{
			Flow flow = ipcManager.allocateFlowResponse(event, 0, true);
			flowAllocated(flow);
		}catch(Exception ex){
			ex.printStackTrace();
		}
	}
	
	public void dispatchFlowDeallocatedEvent(FlowDeallocatedEvent event){
		flowDeallocated(event.getPortId());
	}
}
