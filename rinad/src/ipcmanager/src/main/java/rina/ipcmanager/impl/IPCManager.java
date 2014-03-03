package rina.ipcmanager.impl;

import eu.irati.librina.AllocateFlowResponseEvent;
import eu.irati.librina.ApplicationManagerSingleton;
import eu.irati.librina.ApplicationProcessNamingInformation;
import eu.irati.librina.ApplicationRegistrationRequestEvent;
import eu.irati.librina.ApplicationUnregistrationRequestEvent;
import eu.irati.librina.AssignToDIFResponseEvent;
import eu.irati.librina.CreateIPCProcessException;
import eu.irati.librina.DIFConfiguration;
import eu.irati.librina.EnrollToDIFResponseEvent;
import eu.irati.librina.FlowDeallocateRequestEvent;
import eu.irati.librina.FlowDeallocatedEvent;
import eu.irati.librina.FlowRequestEvent;
import eu.irati.librina.IPCEvent;
import eu.irati.librina.IPCEventProducerSingleton;
import eu.irati.librina.IPCEventType;
import eu.irati.librina.IPCProcess;
import eu.irati.librina.IPCProcessDaemonInitializedEvent;
import eu.irati.librina.IPCProcessFactorySingleton;
import eu.irati.librina.InitializationException;
import eu.irati.librina.IpcmAllocateFlowRequestResultEvent;
import eu.irati.librina.IpcmDeallocateFlowResponseEvent;
import eu.irati.librina.IpcmRegisterApplicationResponseEvent;
import eu.irati.librina.IpcmUnregisterApplicationResponseEvent;
import eu.irati.librina.NeighborsModifiedNotificationEvent;
import eu.irati.librina.OSProcessFinalizedEvent;
import eu.irati.librina.QueryRIBResponseEvent;
import eu.irati.librina.UpdateDIFConfigurationResponseEvent;
import eu.irati.librina.rina;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import com.fasterxml.jackson.core.util.DefaultPrettyPrinter;
import com.fasterxml.jackson.databind.ObjectMapper;

import rina.utils.LogHelper;
import rina.configuration.IPCProcessToCreate;
import rina.configuration.NeighborData;
import rina.configuration.RINAConfiguration;
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
	
	public static final long CONFIG_FILE_POLL_PERIOD_IN_MS = 5000;
	
	public static final String NORMAL_IPC_PROCESS_TYPE = "normal-ipc";
	public static final String SHIM_ETHERNET_IPC_PROCESS_TYPE = "shim-eth-vlan";
	public static final String SHIM_DUMMY_IPC_PROCESS_TYPE = "shim-dummy";
	
	public static final int BOOTSTRAP_PHASE_RESPONSE_WAIT_TIME = 2;
	public static final long NO_IPC_PROCESS_ID = -1;
	public static final long NO_SEQ_NUMBER = -1;
	public static final long ERROR = -1;
	
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
		log.info("Initializing IPCManager console...");
		executorService = Executors.newCachedThreadPool();
		console = new IPCManagerConsole(this);
		executorService.execute(console);
		log.info("IPC Manager daemon initializing, reading RINA configuration ...");
		initializeConfiguration();
		log.info("Initializing librina-ipcmanager...");
		try{
			rina.initializeIPCManager(1, 
					RINAConfiguration.getInstance().getLocalConfiguration().getInstallationPath(), 
					RINAConfiguration.getInstance().getLocalConfiguration().getLibraryPath(), 
					LogHelper.getLibrinaLogLevel(), LogHelper.getLibrinaLogFile());
		}catch(InitializationException ex){
			log.fatal("Error initializing IPC Manager: "+ex.getMessage() 
					+ ". Exiting.");
		}
		
		ipcProcessFactory = rina.getIpcProcessFactory();
		applicationManager = rina.getApplicationManager();
		ipcEventProducer = rina.getIpcEventProducer();
		ipcProcessManager = new IPCProcessManager(ipcProcessFactory, console);
		applicationRegistrationManager = new ApplicationRegistrationManager(
				ipcProcessManager, applicationManager);
		ipcProcessManager.setApplicationRegistrationManager(applicationRegistrationManager);
		flowManager = new FlowManager(ipcProcessManager, applicationManager);
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
				File file = new File(System.getProperty("configFileLocation"));

				while(true){
					if (file.lastModified() > currentLastModified) {
						log.debug("Configuration file changed, loading the new content");
						currentLastModified = file.lastModified();
						rinaConfiguration = readConfigurationFile();
						if (rinaConfiguration != null) {
							RINAConfiguration.setConfiguration(rinaConfiguration);
						}
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
    			objectMapper.readValue(new FileInputStream(System.getProperty("configFileLocation")), 
    					RINAConfiguration.class);
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
	public void bootstrap(){
		RINAConfiguration configuration = RINAConfiguration.getInstance();
		IPCProcessToCreate ipcProcessToCreate = null;
		ApplicationProcessNamingInformation processNamingInfo = null;
		IPCProcess ipcProcess = null;
		List<String> difsToRegisterAt = null;
		List<NeighborData> neighbors = null;
		IPCEvent event = null;
		Map<IPCProcess, List<NeighborData>> pendingEnrollments = new HashMap<IPCProcess, List<NeighborData>>();
		
		for(int i=0; i<configuration.getIpcProcessesToCreate().size(); i++){
			ipcProcessToCreate = configuration.getIpcProcessesToCreate().get(i);
			processNamingInfo = new ApplicationProcessNamingInformation();
			processNamingInfo.setProcessName(ipcProcessToCreate.getApplicationProcessName());
			processNamingInfo.setProcessInstance(ipcProcessToCreate.getApplicationProcessInstance());
			
			try{
				ipcProcess = createIPCProcess(processNamingInfo, 
						ipcProcessToCreate.getType());
				if (ipcProcess.getType().equals(NORMAL_IPC_PROCESS_TYPE)) {
					//Wait for IPC Process Daemon initialized event
					event = ipcEventProducer.eventWait();
					if (event.getType().equals(IPCEventType.IPC_PROCESS_DAEMON_INITIALIZED_EVENT)) {
						IPCProcessDaemonInitializedEvent ipcEvent = (IPCProcessDaemonInitializedEvent) event;
						if (ipcEvent.getIPCProcessId() != ipcProcess.getId()) {
							log.error("Expected IPC Process id "+ipcProcess.getId() 
									+ " but got " + ipcEvent.getIPCProcessId());
							continue;
						}
						
						ipcProcess.setInitialized();
					} else {
						log.error("Expected IPC Process Daemon Initialized event, but got "+event.getType());
						continue;
					}
				}
			}catch(CreateIPCProcessException ex){
				log.error(ex.getMessage() + ". Problems creating IPC Process " 
						+ processNamingInfo.toString());
				continue;
			}
			
			if (ipcProcessToCreate.getDifName() != null){
				try{
					ipcProcessManager.requestAssignToDIF(ipcProcess.getId(), ipcProcessToCreate.getDifName());
					event = ipcEventProducer.eventWait();
					if (event.getType().equals(IPCEventType.ASSIGN_TO_DIF_RESPONSE_EVENT)) {
						AssignToDIFResponseEvent atrEvent = (AssignToDIFResponseEvent) event;
						ipcProcessManager.assignToDIFResponse(atrEvent);
					} else {
						log.error("Problems assigning IPC Process to DIF. Expected event of type " + 
									"ASSIGN_TO_DIF_RESPONSE, but got event of type "+ event.getType());
						continue;
					}
				}catch(Exception ex){
					log.error(ex.getMessage() + ". Problems assigning IPC Process to DIF " 
							+ ipcProcessToCreate.getDifName());
					continue;
				}
			}
			
			difsToRegisterAt = ipcProcessToCreate.getDifsToRegisterAt();
			if (difsToRegisterAt != null && difsToRegisterAt.size() > 0){
				for(int j=0; j<difsToRegisterAt.size(); j++){
					try{
						requestRegistrationToNMinusOneDIF(
								ipcProcess.getId(), difsToRegisterAt.get(j));
						event = ipcEventProducer.eventWait();
						if (event.getType().equals(IPCEventType.IPCM_REGISTER_APP_RESPONSE_EVENT)) {
							IpcmRegisterApplicationResponseEvent appRespEvent = (IpcmRegisterApplicationResponseEvent) event;
							applicationRegistrationManager.registerApplicationResponse(appRespEvent);
						} else {
							log.error("Problems assigning IPC Process to DIF. Expected event of type " + 
										"IPCM_REGISTER_APP_RESPONSE_EVENT, but got event of type "+ event.getType());
							continue;
						}
					}catch(Exception ex){
						log.error("Error requesting registration of IPC Process " + ipcProcess.getId() 
								+" to DIF " + difsToRegisterAt.get(j) + ": "+ex.getMessage());
					}
				}
			}
			
			neighbors = ipcProcessToCreate.getNeighbors();
			if (neighbors != null && neighbors.size() > 0){
				pendingEnrollments.put(ipcProcess, neighbors);
			}
		}
		
		if (!pendingEnrollments.isEmpty()) {
			Iterator<Entry<IPCProcess, List<NeighborData>>> iterator = pendingEnrollments.entrySet().iterator();
			Entry<IPCProcess, List<NeighborData>> currentEntry = null;
			NeighborData currentNeighbor = null;
			while (iterator.hasNext()) {
				currentEntry = iterator.next();
				neighbors = currentEntry.getValue();
				ipcProcess = currentEntry.getKey();
				for(int i=0; i<neighbors.size(); i++) {
					currentNeighbor = neighbors.get(i);
					try{
						requestEnrollmentToDIF(
								ipcProcess.getId(), currentNeighbor.getDifName(), 
								currentNeighbor.getSupportingDifName(), 
								currentNeighbor.getApName(), 
								currentNeighbor.getApInstance());
					}catch(Exception ex){
						log.error("Error requesting enrollment of IPC Process " + ipcProcess.getId() 
								+ " to neighbor " + currentNeighbor.getApName() 
								+ " using N-1 DIF "+currentNeighbor.getSupportingDifName());
					}
				}
			}
		}
	}
	
	public void executeEventLoop(){
		IPCEvent event = null;
		
		while(true){
			try{
				event = ipcEventProducer.eventWait();
				processEvent(event);
			}catch(Exception ex){
				log.error("Problems processing event of type " + event.getType() + 
						". " + ex.getMessage());
			}
		}
	}
	
	private void processEvent(IPCEvent event) throws Exception{
		log.info("Got event of type: "+event.getType() 
				+ " and sequence number: "+event.getSequenceNumber());

		switch(event.getType()) {
		case APPLICATION_REGISTRATION_REQUEST_EVENT:
			ApplicationRegistrationRequestEvent appRegReqEvent = (ApplicationRegistrationRequestEvent) event;
			applicationRegistrationManager.requestApplicationRegistration(appRegReqEvent, NO_IPC_PROCESS_ID);
			break;
		case IPCM_REGISTER_APP_RESPONSE_EVENT:
			IpcmRegisterApplicationResponseEvent appRespEvent = (IpcmRegisterApplicationResponseEvent) event;
			applicationRegistrationManager.registerApplicationResponse(appRespEvent);
			break;
		case APPLICATION_UNREGISTRATION_REQUEST_EVENT:
			ApplicationUnregistrationRequestEvent appUnregReqEvent = (ApplicationUnregistrationRequestEvent) event;
			applicationRegistrationManager.requestApplicationUnregistration(appUnregReqEvent, IPCManager.NO_IPC_PROCESS_ID);
			break;
		case IPCM_UNREGISTER_APP_RESPONSE_EVENT:
			IpcmUnregisterApplicationResponseEvent appUnRespEvent = (IpcmUnregisterApplicationResponseEvent) event;
			applicationRegistrationManager.unregisterApplicationResponse(appUnRespEvent);
			break;
		case FLOW_ALLOCATION_REQUESTED_EVENT:
			FlowRequestEvent flowReqEvent = (FlowRequestEvent) event;
			if (flowReqEvent.isLocalRequest()){
				flowManager.requestAllocateFlowLocal(flowReqEvent);
			}else{
				flowManager.allocateFlowRemote(flowReqEvent);
			}
			break;
		case IPCM_ALLOCATE_FLOW_REQUEST_RESULT:
			IpcmAllocateFlowRequestResultEvent flowEvent = (IpcmAllocateFlowRequestResultEvent) event;
			flowManager.allocateFlowRequestResult(flowEvent);
			break;
		case ALLOCATE_FLOW_RESPONSE_EVENT:
			AllocateFlowResponseEvent flowRespEvent = (AllocateFlowResponseEvent) event;
			flowManager.allocateFlowResponse(flowRespEvent);
			break;
		case FLOW_DEALLOCATION_REQUESTED_EVENT:
			FlowDeallocateRequestEvent flowDeReqEvent = (FlowDeallocateRequestEvent) event;
			flowManager.deallocateFlowRequest(flowDeReqEvent);
			break;
		case IPCM_DEALLOCATE_FLOW_RESPONSE_EVENT:
			IpcmDeallocateFlowResponseEvent flowDeEvent = (IpcmDeallocateFlowResponseEvent) event;
			flowManager.deallocateFlowResponse(flowDeEvent);
			break;
		case FLOW_DEALLOCATED_EVENT:
			FlowDeallocatedEvent flowDeaEvent = (FlowDeallocatedEvent) event;
			flowManager.flowDeallocated(flowDeaEvent);
			break;
		case GET_DIF_PROPERTIES:
			break;
		case OS_PROCESS_FINALIZED:
			OSProcessFinalizedEvent osProcessFinalizedEvent = (OSProcessFinalizedEvent) event;
			processOSProcessFinalizedEvent(osProcessFinalizedEvent);
			break;
		case ASSIGN_TO_DIF_RESPONSE_EVENT:
			AssignToDIFResponseEvent atrEvent = (AssignToDIFResponseEvent) event;
			ipcProcessManager.assignToDIFResponse(atrEvent);
			console.responseArrived(event);
			break;
		case UPDATE_DIF_CONFIG_RESPONSE_EVENT:
			UpdateDIFConfigurationResponseEvent updateConfEvent = (UpdateDIFConfigurationResponseEvent) event;
			ipcProcessManager.updateDIFConfigurationResponse(updateConfEvent);
			break;
		case IPC_PROCESS_DAEMON_INITIALIZED_EVENT:
			IPCProcessDaemonInitializedEvent ipcEvent = (IPCProcessDaemonInitializedEvent) event;
			ipcProcessManager.setInitialized(ipcEvent.getIPCProcessId());
			break;
		case ENROLL_TO_DIF_RESPONSE_EVENT:
			EnrollToDIFResponseEvent enrollResponseEvent = (EnrollToDIFResponseEvent) event;
			ipcProcessManager.enrollToDIFResponse(enrollResponseEvent);
			break;
		case NEIGHBORS_MODIFIED_NOTIFICAITON_EVENT:
			NeighborsModifiedNotificationEvent neighborsEvent = (NeighborsModifiedNotificationEvent) event;
			ipcProcessManager.neighborsModifiedEvent(neighborsEvent);
			break;
		case QUERY_RIB_RESPONSE_EVENT:
			QueryRIBResponseEvent queryRIBEvent = (QueryRIBResponseEvent) event;
			console.responseArrived(queryRIBEvent);
			break;
		default:
			log.warn("Got unsupported event type: "+event.getType());
			break;
		}
	}
	
	private void processOSProcessFinalizedEvent(OSProcessFinalizedEvent event) {
		log.info("OS process finalized. Naming info: " 
				+ event.getApplicationName().toString());

		//Clean up all stuff related to the finalized process (registrations, flows)
		flowManager.cleanFlows(event.getApplicationName());
		applicationRegistrationManager.cleanApplicationRegistrations(
				event.getApplicationName());

		if (event.getIPCProcessId() != 0){
			//TODO The process that crashed was an IPC Process in user space
			//Should we destroy the state in the kernel? Or try to create another
			//IPC Process in user space to bring it back?
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
	public void destroyIPCProcess(int ipcProcessId) throws Exception{
		//TODO if the IPC Process exists, delete all flows that go through the IPC Process,
		//and terminate all application registrations.
		
		//Destroy the IPC Process
		ipcProcessManager.destroyIPCProcess(ipcProcessId);
	}
	
	/**
	 * Requests the assignment of the IPC Process identified by ipcProcessID to the 
	 * DIF called 'difName'. The DIF configuration must be available in the IPC Manager 
	 * configuration file, otherwise this operation will return an error
	 * @param ipcProcessID
	 * @param difName
	 * @throws Exception
	 */
	public long requestAssignToDIF(int ipcProcessID, String difName) throws Exception{
		return ipcProcessManager.requestAssignToDIF(ipcProcessID, difName);
	}
	
	
	/**
	 * Updates the configuration of the IPC Process identified by 'ipcProcessId'
	 * @param ipcProcessID
	 * @param difConfiguration
	 * @throws Exception
	 */
	public long requestUpdateDIFConfiguration(int ipcProcessID,
				DIFConfiguration difConfiguration) throws Exception{
		 return ipcProcessManager.requestUpdateDIFConfiguration(ipcProcessID, difConfiguration);
	}

	/**
	 * Requests the registration of the IPC Process identified by ipcProcessID to the 
	 * N-1 DIF called 'difName'. If the IPC Process is already registered to the N-1 DIF
	 * an error will be returned
	 * @param ipcProcessID
	 * @param difName
	 * @throws Exception
	 */
	public long requestRegistrationToNMinusOneDIF(int ipcProcessID, 
			String difName) throws Exception{
		return ipcProcessManager.requestRegistrationToNMinusOneDIF(ipcProcessID, difName);
	}
	
	/**
	 * Requests the unregistration of the IPC Process identified by ipcProcessID from the 
	 * N-1 DIF called 'difName'. If the IPC Process is not registered to the N-1 DIF
	 * an error will be returned
	 * @param ipcProcessID
	 * @param difName
	 * @throws Exception
	 */
	public long requestUnregistrationFromNMinusOneDIF(int ipcProcessID, 
			String difName) throws Exception{
		return ipcProcessManager.requestUnregistrationFromNMinusOneDIF(ipcProcessID, difName);
	}
	
	/**
	 * Requests the enrollment of the IPC Process identified by ipcProcessID to a DIF 
	 * through a neighbor using a supportingDIF. The IPC Process may or may not be 
	 * already a member of the DIF.
	 * @param ipcProcessId
	 * @param difName
	 * @param supportingDifName
	 * @param neighApName
	 * @param neighApInstance
	 * @return
	 * @throws Exception
	 */
	public long requestEnrollmentToDIF(int ipcProcessId, String difName, 
			String supportingDifName, String neighApName, String neighApInstance) throws Exception {
		return ipcProcessManager.requestEnrollmentToDIF(ipcProcessId, difName, 
				supportingDifName, neighApName, neighApInstance);
	}
	
	/**
	 * Query the RIB of an IPC Process. Right now only querying the full RIB is 
	 * supported
	 * @param ipcProcessId
	 * @return
	 * @throws Exception
	 */
	public long requestQueryIPCProcessRIB(int ipcProcessId) throws Exception {
		return ipcProcessManager.requestQueryRIB(ipcProcessId);
	}
}
