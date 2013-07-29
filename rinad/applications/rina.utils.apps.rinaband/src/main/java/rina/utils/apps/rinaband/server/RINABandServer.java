package rina.utils.apps.rinaband.server;

import java.util.Hashtable;
import java.util.Map;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

import eu.irati.librina.ApplicationProcessNamingInformation;
import eu.irati.librina.ApplicationRegistration;
import eu.irati.librina.Flow;
import eu.irati.librina.FlowDeallocatedEvent;
import eu.irati.librina.FlowRequestEvent;
import eu.irati.librina.IPCManagerSingleton;
import eu.irati.librina.rina;

import rina.cdap.api.CDAPSessionManager;
import rina.cdap.impl.CDAPSessionManagerImpl;
import rina.cdap.impl.googleprotobuf.GoogleProtocolBufWireMessageProviderFactory;
import rina.utils.apps.rinaband.utils.FlowAcceptor;
import rina.utils.apps.rinaband.utils.FlowDeallocationListener;
import rina.utils.apps.rinaband.utils.FlowReader;
import rina.utils.apps.rinaband.utils.IPCEventConsumer;

/**
 * Implements the behavior of a RINABand Server
 * @author eduardgrasa
 *
 */
public class RINABandServer implements FlowAcceptor, FlowDeallocationListener{
	
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
	
	public RINABandServer(ApplicationProcessNamingInformation controlApNamingInfo, 
			ApplicationProcessNamingInformation dataApNamingInfo){
		this.controlApNamingInfo = controlApNamingInfo;
		this.dataApNamingInfo = dataApNamingInfo;
		ongoingTests = new Hashtable<Integer, TestController>();
		cdapSessionManager = new CDAPSessionManagerImpl(new GoogleProtocolBufWireMessageProviderFactory());
		ipcEventConsumer = new IPCEventConsumer();
		ipcEventConsumer.addFlowAcceptor(this, controlApNamingInfo);
		executeRunnable(ipcEventConsumer);
	}
	
	public synchronized static void executeRunnable(Runnable runnable){
		executorService.execute(runnable);
	}
	
	public void execute(){
		//Register the control AE and wait for new RINABand clients to come
		try{
			ApplicationRegistration appRegistration = rina.getIpcManager().registerApplication(controlApNamingInfo, null);
			System.out.println("Registered applicaiton "+controlApNamingInfo.toString() + 
					" to DIF " + appRegistration.getDIFNames().getFirst().toString());
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
		TestController testController = new TestController(dataApNamingInfo, flow, 
				this.cdapSessionManager, ipcEventConsumer);
		FlowReader flowReader = new FlowReader(flow, testController, 10000);
		RINABandServer.executeRunnable(flowReader);
		ongoingTests.put(new Integer(flow.getPortId()), testController);
		ipcEventConsumer.addFlowDeallocationListener(this, flow.getPortId());
		System.out.println("New flow to the control AE allocated, with port id "+flow.getPortId());
	}

	/**
	 * Called when the control flow with the RINABand client is deallocated
	 */
	public synchronized void flowDeallocated(int portId) {
		ongoingTests.remove(new Integer(portId));
		ipcEventConsumer.removeFlowDeallocationListener(portId);
		System.out.println("Control flow with port id "+portId+" deallocated");
	}

	/**
	 * Decide when a flow can be accepted
	 */
	public void dispatchFlowRequestEvent(FlowRequestEvent event){
		IPCManagerSingleton ipcManager = rina.getIpcManager();
		if (this.ongoingTests.size() >= MAX_CONCURRENT_TESTS){
			try{
				ipcManager.allocateFlowResponse(
						event, false, "Cannot execute more concurrent tests now. Try later");
			}catch(Exception ex){
				ex.printStackTrace();
			}
			return;
		}

		try{
			Flow flow = ipcManager.allocateFlowResponse(event, true, "ok");
			flowAllocated(flow);
		}catch(Exception ex){
			ex.printStackTrace();
		}
	}
	
	public void dispatchFlowDeallocatedEvent(FlowDeallocatedEvent event){
		flowDeallocated(event.getPortId());
	}
}
