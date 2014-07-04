package rina.utils.apps.echo.server;

import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import rina.utils.LogHelper;
import rina.cdap.api.CDAPSessionManager;
import rina.cdap.impl.CDAPSessionManagerImpl;
import rina.cdap.impl.googleprotobuf.GoogleProtocolBufWireMessageProviderFactory;
import rina.utils.apps.echo.utils.ApplicationRegistrationListener;
import rina.utils.apps.echo.utils.FlowAcceptor;
import rina.utils.apps.echo.utils.FlowDeallocationListener;
import rina.utils.apps.echo.utils.IPCEventConsumer;
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

/**
 * Implements the behavior of a RINABand Server
 * @author eduardgrasa
 *
 */
public class EchoServer implements FlowAcceptor, ApplicationRegistrationListener, FlowDeallocationListener {
	
	public static final int MAX_SDU_SIZE_IN_BYTES = 10000;
	public static final int MAX_SDUS_PER_FLOW = 1000000;
	
	/**
	 * The APNamingInfo associated to the control AE of the RINABand application
	 */
	private ApplicationProcessNamingInformation echoApNamingInfo = null;
	
	/** The name of the DIF where the RINABandServer has registered */
	private ApplicationProcessNamingInformation difName = null;
	
	private IPCEventConsumer ipcEventConsumer = null;
	
	private static ExecutorService executorService = Executors.newCachedThreadPool();
	
	private static final Log log = LogFactory.getLog(EchoServer.class);
	
	private Map<Integer, TestController> ongoingTests = null;
	
	/**
	 * Manages the CDAP sessions to the control AE
	 */
	private CDAPSessionManager cdapSessionManager = null;
	
	//private int timeout = 0;
	
	public EchoServer(ApplicationProcessNamingInformation echoApNamingInfo){
		try {
			rina.initialize(LogHelper.getLibrinaLogLevel(), 
					LogHelper.getLibrinaLogFile());
		} catch(Exception ex){
			log.error("Problems initializing librina, exiting: "+ex.getMessage());
			System.exit(-1);
		}
		
		this.cdapSessionManager = new CDAPSessionManagerImpl(
				new GoogleProtocolBufWireMessageProviderFactory());
		this.echoApNamingInfo = echoApNamingInfo;
		this.ongoingTests = new ConcurrentHashMap<Integer, TestController>();
		ipcEventConsumer = new IPCEventConsumer();
		ipcEventConsumer.addFlowAcceptor(this, echoApNamingInfo);
		ipcEventConsumer.addApplicationRegistrationListener(this, echoApNamingInfo);
		executeRunnable(ipcEventConsumer);
	}
	
	public synchronized static void executeRunnable(Runnable runnable){
		executorService.execute(runnable);
	}
	
	public void execute(){
		//Register the echo AE and wait for new Echo client to come
		try{
			ApplicationRegistrationInformation appRegInfo = 
					new ApplicationRegistrationInformation(
							ApplicationRegistrationType.APPLICATION_REGISTRATION_ANY_DIF);
			appRegInfo.setApplicationName(echoApNamingInfo);
			rina.getIpcManager().requestApplicationRegistration(appRegInfo);
			log.info("Requested registration of control AE: "+echoApNamingInfo.toString());
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
				log.info("Succesfully registered AE " + event.getApplicationName().toString() 
						+ " to DIF" + difName.getProcessName());
			} catch (Exception ex){
				log.error(ex.getMessage());
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

	/**
	 * Called when a new RINABand client allocates a flow to the control AE 
	 * of the RINABand Server, in order to negotiate the new test parameters
	 */
	public synchronized void flowAllocated(Flow flow) {
		TestController flowReader = new TestController(flow, 10000, cdapSessionManager);
		ongoingTests.put(flow.getPortId(), flowReader);
		ipcEventConsumer.addFlowDeallocationListener(this, flow.getPortId());
		EchoServer.executeRunnable(flowReader);
		log.info("New flow to the echo AE allocated, with port id "+flow.getPortId());
	}

	/**
	 * Decide when a flow can be accepted
	 */
	public void dispatchFlowRequestEvent(FlowRequestEvent event){
		IPCManagerSingleton ipcManager = rina.getIpcManager();

		try{
			Flow flow = ipcManager.allocateFlowResponse(event, 0, true);
			flowAllocated(flow);
		}catch(Exception ex){
			ex.printStackTrace();
		}
	}
	
	@Override
	public void dispatchFlowDeallocatedEvent(FlowDeallocatedEvent event) {
		ipcEventConsumer.removeFlowDeallocationListener(event.getPortId());
		TestController flowReader = ongoingTests.remove(event.getPortId());
		if (flowReader == null){
			log.warn("Flowreader of flow "+event.getPortId()
					+" not found in ongoing tests table");
			return;
		}
		
		log.info("Requesting Flowreader of flow "+event.getPortId() 
				+ " to stop");
		flowReader.stop();
	}
}
