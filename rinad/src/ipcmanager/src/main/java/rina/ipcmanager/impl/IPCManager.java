package rina.ipcmanager.impl;

import eu.irati.librina.ApplicationManagerSingleton;
import eu.irati.librina.ApplicationProcessNamingInformation;
import eu.irati.librina.ApplicationRegistrationRequestEvent;
import eu.irati.librina.ApplicationUnregistrationRequestEvent;
import eu.irati.librina.AssignToDIFResponseEvent;
import eu.irati.librina.CreateIPCProcessException;
import eu.irati.librina.DIFConfiguration;
import eu.irati.librina.FlowDeallocateRequestEvent;
import eu.irati.librina.FlowDeallocatedEvent;
import eu.irati.librina.FlowRequestEvent;
import eu.irati.librina.IPCEvent;
import eu.irati.librina.IPCEventProducerSingleton;
import eu.irati.librina.IPCEventType;
import eu.irati.librina.IPCManagerInitializationException;
import eu.irati.librina.IPCProcess;
import eu.irati.librina.IPCProcessFactorySingleton;
import eu.irati.librina.IpcmRegisterApplicationResponseEvent;
import eu.irati.librina.IpcmUnregisterApplicationResponseEvent;
import eu.irati.librina.OSProcessFinalizedEvent;
import eu.irati.librina.rina;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.util.Iterator;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import com.fasterxml.jackson.core.util.DefaultPrettyPrinter;
import com.fasterxml.jackson.databind.ObjectMapper;

import rina.ipcmanager.impl.conf.IPCProcessToCreate;
import rina.ipcmanager.impl.conf.RINAConfiguration;
import rina.ipcmanager.impl.console.IPCManagerConsole;
import rina.ipcmanager.impl.helpers.ApplicationRegistrationManager;
import rina.ipcmanager.impl.helpers.FlowManager;
import rina.ipcmanager.impl.helpers.IPCProcessManager;

/**
 * The IPC Manager is the component of a DAF that manages the local IPC 
 * resources. In its current implementation it manages IPC Processes 
 * (creates/configures/ destroys them), and serves as a broker between 
 * applications and IPC Processes. Applications can use the RINA library 
 * to establish a connection to the IPC Manager and interact with the RINA 
 * stack.
 * @author eduardgrasa
 *
 */
public class IPCManager {
	
	private static final Log log = LogFactory.getLog(IPCManager.class);
	
	public static final String CONFIG_FILE_LOCATION = "../conf/ipcmanager.conf"; 
	public static final long CONFIG_FILE_POLL_PERIOD_IN_MS = 5000;
	
	public static final String NORMAL_IPC_PROCESS_TYPE = "normal";
	public static final String SHIM_ETHERNET_IPC_PROCESS_TYPE = "shim-eth-vlan";
	public static final String SHIM_DUMMY_IPC_PROCESS_TYPE = "shim-dummy";
	
	private IPCManagerConsole console = null;
	
	/**
	 * The thread pool implementation
	 */
	private ExecutorService executorService = null;
	
	private IPCProcessFactorySingleton ipcProcessFactory = null;
	private ApplicationManagerSingleton applicationManager = null;
	private IPCEventProducerSingleton ipcEventProducer = null;
	
	private IPCProcessManager ipcProcessManager = null;
	private ApplicationRegistrationManager applicationRegistrationManager = null;
	private FlowManager flowManager = null;
	
	public IPCManager(){
		log.info("Initializing IPCManager console..,");
		executorService = Executors.newCachedThreadPool();
		console = new IPCManagerConsole(this);
		executorService.execute(console);
		log.info("IPC Manager daemon initializing, reading RINA configuration ...");
		initializeConfiguration();
		log.info("Initializing librina-ipcmanager...");
		try{
			rina.initializeIPCManager(1, 
					RINAConfiguration.getInstance().getLocalConfiguration().getInstallationPath(), 
					RINAConfiguration.getInstance().getLocalConfiguration().getLibraryPath());
		}catch(IPCManagerInitializationException ex){
			log.fatal("Error initializing IPC Manager: "+ex.getMessage() 
					+ ". Exiting.");
		}
		ipcProcessFactory = rina.getIpcProcessFactory();
		applicationManager = rina.getApplicationManager();
		ipcEventProducer = rina.getIpcEventProducer();
		ipcProcessManager = new IPCProcessManager(ipcProcessFactory);
		applicationRegistrationManager = new ApplicationRegistrationManager(
				ipcProcessManager, applicationManager);
		flowManager = new FlowManager(ipcProcessFactory, applicationManager);
		
		log.info("Bootstrapping IPC Manager ...");
		bootstrap();
	}
	
	private void initializeConfiguration(){
		//Read config file
		RINAConfiguration rinaConfiguration = readConfigurationFile();
		RINAConfiguration.setConfiguration(rinaConfiguration);
		
		//Start thread that will look for config file changes
		Runnable configFileChangeRunnable = new Runnable(){
			private long currentLastModified = System.currentTimeMillis();
			private RINAConfiguration rinaConfiguration = null;

			public void run(){
				File file = new File(CONFIG_FILE_LOCATION);

				while(true){
					if (file.lastModified() > currentLastModified) {
						log.debug("Configuration file changed, loading the new content");
						currentLastModified = file.lastModified();
						rinaConfiguration = readConfigurationFile();
						RINAConfiguration.setConfiguration(rinaConfiguration);
					}
					try {
						Thread.sleep(CONFIG_FILE_POLL_PERIOD_IN_MS);
					}catch (InterruptedException e) {
						e.printStackTrace();
					}
				}
			}

		};

		executorService.execute(configFileChangeRunnable);
	}
	
	private RINAConfiguration readConfigurationFile(){
		try {
    		ObjectMapper objectMapper = new ObjectMapper();
    		RINAConfiguration rinaConfiguration = (RINAConfiguration) 
    			objectMapper.readValue(new FileInputStream(CONFIG_FILE_LOCATION), RINAConfiguration.class);
    		log.info("Read configuration file");
    		ByteArrayOutputStream outputStream = new ByteArrayOutputStream();
    		objectMapper.writer(new DefaultPrettyPrinter()).writeValue(outputStream, rinaConfiguration);
    		log.info(outputStream.toString());
    		return rinaConfiguration;
    	} catch (Exception ex) {
    		log.error(ex);
    		ex.printStackTrace();
    		log.debug("Could not find the main configuration file.");
    		return null;
        }
	}
	
	/**
	 * Read the configuration file and create IPC processes, assign them 
	 * to DIFs, cause them to enroll to other IPC processes etc, as 
	 * specified in the config file.
	 */
	private void bootstrap(){
		RINAConfiguration configuration = RINAConfiguration.getInstance();
		IPCProcessToCreate ipcProcessToCreate = null;
		ApplicationProcessNamingInformation processNamingInfo = null;
		IPCProcess ipcProcess = null;
		
		for(int i=0; i<configuration.getIpcProcessesToCreate().size(); i++){
			ipcProcessToCreate = configuration.getIpcProcessesToCreate().get(i);
			processNamingInfo = new ApplicationProcessNamingInformation();
			processNamingInfo.setProcessName(ipcProcessToCreate.getApplicationProcessName());
			processNamingInfo.setProcessInstance(ipcProcessToCreate.getApplicationProcessInstance());
			
			try{
				ipcProcess = createIPCProcess(processNamingInfo, 
						ipcProcessToCreate.getType());
			}catch(CreateIPCProcessException ex){
				log.error(ex.getMessage() + ". Problems creating IPC Process " 
						+ processNamingInfo.toString());
				continue;
			}
			
			if (ipcProcessToCreate.getDifName() != null){
				try{
					ipcProcessManager.requestAssignToDIF(ipcProcess.getId(), ipcProcessToCreate.getDifName());
				}catch(Exception ex){
					log.error(ex.getMessage() + ". Problems assigning IPC Process to DIF " 
							+ ipcProcessToCreate.getDifName());
					continue;
				}
			}
			
			if (ipcProcessToCreate.getDifsToRegisterAt() != null && 
					ipcProcessToCreate.getDifsToRegisterAt().size() > 0){
				//TODO register in underlying DIFs
			}
			
			if (ipcProcessToCreate.getNeighbors() != null && 
					ipcProcessToCreate.getNeighbors().size() > 0){
				//TODO cause enrollment to be initiated
			}
		}
	}
	
	public void startEventLoopWorkers(){
		int eventLoopWorkers = 
				RINAConfiguration.getInstance().getLocalConfiguration().getEventLoopWorkers();
		log.info("Starting " + eventLoopWorkers + " event loop workers");
		
		Runnable worker = null;
		for(int i=0; i<eventLoopWorkers; i++){
			worker = new IPCManagerEventLoopWorker(this);
			executorService.execute(worker);
		}
	}
	
	public void executeEventLoop(){
		IPCEvent event = null;
		
		while(true){
			try{
				event = ipcEventProducer.eventWait();
				processEvent(event);
			}catch(Exception ex){
				ex.printStackTrace();
			}
		}
	}
	
	private void processEvent(IPCEvent event) throws Exception{
		log.info("Got event of type: "+event.getType() 
				+ " and sequence number: "+event.getSequenceNumber());
		
		if (event.getType() == IPCEventType.APPLICATION_REGISTRATION_REQUEST_EVENT) {
			ApplicationRegistrationRequestEvent appRegReqEvent = (ApplicationRegistrationRequestEvent) event;
			applicationRegistrationManager.requestApplicationRegistration(appRegReqEvent);
		} else if (event.getType() == IPCEventType.IPCM_REGISTER_APP_RESPONSE_EVENT) {
			IpcmRegisterApplicationResponseEvent appRespEvent = (IpcmRegisterApplicationResponseEvent) event;
			applicationRegistrationManager.registerApplicationResponse(appRespEvent);
		} else if (event.getType() == IPCEventType.APPLICATION_UNREGISTRATION_REQUEST_EVENT){
			ApplicationUnregistrationRequestEvent appUnregReqEvent = (ApplicationUnregistrationRequestEvent) event;
			applicationRegistrationManager.requestApplicationUnregistration(appUnregReqEvent);
		} else if (event.getType() == IPCEventType.IPCM_UNREGISTER_APP_RESPONSE_EVENT) {
			IpcmUnregisterApplicationResponseEvent appRespEvent = (IpcmUnregisterApplicationResponseEvent) event;
			applicationRegistrationManager.unregisterApplicationResponse(appRespEvent);
		} else if (event.getType() == IPCEventType.FLOW_ALLOCATION_REQUESTED_EVENT){
			FlowRequestEvent flowReqEvent = (FlowRequestEvent) event;
			if (flowReqEvent.isLocalRequest()){
				flowManager.allocateFlowLocal(flowReqEvent);
			}else{
				flowManager.allocateFlowRemote(flowReqEvent);
			}
		}else if (event.getType() == IPCEventType.FLOW_DEALLOCATION_REQUESTED_EVENT){
			FlowDeallocateRequestEvent flowDeReqEvent = (FlowDeallocateRequestEvent) event;
			flowManager.deallocateFlow(flowDeReqEvent);
		}else if (event.getType() == IPCEventType.FLOW_DEALLOCATED_EVENT){
			FlowDeallocatedEvent flowDeEvent = (FlowDeallocatedEvent) event;
			flowManager.flowDeallocated(flowDeEvent);
		}else if (event.getType().equals(IPCEventType.GET_DIF_PROPERTIES)){
			//TODO
		}else if (event.getType().equals(IPCEventType.OS_PROCESS_FINALIZED)){
			OSProcessFinalizedEvent osProcessFinalizedEvent = (OSProcessFinalizedEvent) event;
			log.info("OS process finalized. Naming info: " 
					+ osProcessFinalizedEvent.getApplicationName().toString());
			
			//Clean up all stuff related to the finalized process (registrations, flows)
			applicationRegistrationManager.cleanApplicationRegistrations(
					osProcessFinalizedEvent.getApplicationName());
			flowManager.cleanFlows(osProcessFinalizedEvent.getApplicationName());
			
			if (osProcessFinalizedEvent.getIPCProcessId() != 0){
				//TODO The process that crashed was an IPC Process in user space
				//Should we destroy the state in the kernel? Or try to create another
				//IPC Process in user space to bring it back?
			}
		}else if (event.getType().equals(IPCEventType.ASSIGN_TO_DIF_RESPONSE_EVENT)){
			AssignToDIFResponseEvent atrEvent = (AssignToDIFResponseEvent) event;
			ipcProcessManager.assignToDIFResponse(atrEvent);
		}
	}
	
	public String getIPCProcessesInformationAsString(){
		return ipcProcessManager.getIPCProcessesInformationAsString();
	}
	
	public String getSystemCapabilitiesAsString(){
		String result = "";
		result = result + "Supported IPC Process types:\n";
		Iterator<String> iterator = ipcProcessFactory.getSupportedIPCProcessTypes().iterator();
		while(iterator.hasNext()){
			result = result + "    " + iterator.next() + "\n";
		}
		
		return result;
	}
	
	/**
	 * Requests the creation of an IPC Process
	 * @param name
	 * @param type
	 * @throws CreateIPCProcessException
	 */
	public IPCProcess createIPCProcess(ApplicationProcessNamingInformation name, 
			String type) throws CreateIPCProcessException {
		return ipcProcessManager.createIPCProcess(name, type);
	}
	
	/**
	 * Destroys an IPC Process
	 * @param ipcProcessId
	 */
	public void destroyIPCProcess(long ipcProcessId) throws Exception{
		//TODO if the IPC Process exists, delete all flows that go through the IPC Process,
		//and terminate all application registrations.
		
		//Destroy the IPC Process
		ipcProcessManager.destroyIPCProcess(ipcProcessId);
	}
	
	/**
	 * Assigns the IPC Process identified by ipcProcessID to the DIF called 'difName'
	 * The DIF configuration must be available in the IPC Manager configuration file, 
	 * otherwise this operation will return an error
	 * @param ipcProcessID
	 * @param difName
	 * @throws Exception
	 */
	public void requestAssignToDIF(long ipcProcessID, String difName) throws Exception{
		ipcProcessManager.requestAssignToDIF(ipcProcessID, difName);
	}
	
	
	public void updateDIFConfiguration(long ipcProcessID,
				DIFConfiguration difConfiguration) throws Exception{
		IPCProcess ipcProcess = this.ipcProcessFactory.getIPCProcess(ipcProcessID);
		ipcProcess.updateDIFConfiguration(difConfiguration);
	}

}
