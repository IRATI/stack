package rina.utils.apps.echo.client;

import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

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
	
	private int numberOfSdus = 0;
	private int sduSize = 0;
	
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
	
	public EchoClient(int numberOfSdus, int sduSize, 
			ApplicationProcessNamingInformation echoApNamingInfo){
		rina.initialize();
		this.numberOfSdus = numberOfSdus;
		this.sduSize = sduSize;
		this.echoApNamingInfo = echoApNamingInfo;
		this.clientApNamingInfo = new ApplicationProcessNamingInformation("rina.utils.apps.echoclient", "1");
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
				
				//2 Start flowReader
				FlowReader flowReader = new FlowReader(this.flow, this.sduSize, this.numberOfSdus);
				this.executorService.execute(flowReader);
				
				//3 Send SDUs to server
				byte[] buffer = new byte[sduSize];
				for(int i=0; i<numberOfSdus; i++){
					try{
						flow.writeSDU(buffer, sduSize);
					}catch(Exception ex){
						ex.printStackTrace();
					}
				}
			} else {
				log.error("Problems allocating flow to control AE: " + echoApNamingInfo.toString());
				try{
					rina.getIpcManager().withdrawPendingFlow(event.getSequenceNumber());
				} catch (Exception ex) {
					log.error(ex.getMessage());
					System.exit(-1);
				}
			}
		}
	}

	@Override
	public void dispatchFlowDeallocatedEvent(FlowDeallocatedEvent event) {
		// TODO Auto-generated method stub
		
	}
}
