package rina.utils.apps.echo.server;

import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import rina.utils.apps.echo.utils.FlowAcceptor;
import rina.utils.apps.echo.utils.IPCEventConsumer;
import eu.irati.librina.ApplicationProcessNamingInformation;
import eu.irati.librina.ApplicationRegistration;
import eu.irati.librina.ApplicationRegistrationInformation;
import eu.irati.librina.ApplicationRegistrationType;
import eu.irati.librina.Flow;
import eu.irati.librina.FlowRequestEvent;
import eu.irati.librina.IPCManagerSingleton;
import eu.irati.librina.rina;

/**
 * Implements the behavior of a RINABand Server
 * @author eduardgrasa
 *
 */
public class EchoServer implements FlowAcceptor{
	
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
	
	public EchoServer(ApplicationProcessNamingInformation echoApNamingInfo){
		this.echoApNamingInfo = echoApNamingInfo;
		rina.initialize();
		ipcEventConsumer = new IPCEventConsumer();
		ipcEventConsumer.addFlowAcceptor(this, echoApNamingInfo);
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
			ApplicationRegistration appRegistration = 
					rina.getIpcManager().registerApplication(echoApNamingInfo, appRegInfo);
			difName = appRegistration.getDIFNames().getFirst();
			log.info("Registered application "+echoApNamingInfo.toString() + 
					" to DIF " + difName.toString());
		}catch(Exception ex){
			ex.printStackTrace();
			System.exit(-1);
		}
	}

	/**
	 * Called when a new RINABand client allocates a flow to the control AE 
	 * of the RINABand Server, in order to negotiate the new test parameters
	 */
	public synchronized void flowAllocated(Flow flow) {
		FlowReader flowReader = new FlowReader(flow, 10000);
		EchoServer.executeRunnable(flowReader);
		ipcEventConsumer.addFlowDeallocationListener(flowReader, flow.getPortId());
		log.info("New flow to the echo AE allocated, with port id "+flow.getPortId());
	}

	/**
	 * Decide when a flow can be accepted
	 */
	public void dispatchFlowRequestEvent(FlowRequestEvent event){
		IPCManagerSingleton ipcManager = rina.getIpcManager();

		try{
			Flow flow = ipcManager.allocateFlowResponse(event, true, "ok");
			flowAllocated(flow);
		}catch(Exception ex){
			ex.printStackTrace();
		}
	}
}
