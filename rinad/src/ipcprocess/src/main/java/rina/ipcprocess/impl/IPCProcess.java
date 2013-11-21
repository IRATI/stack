package rina.ipcprocess.impl;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Properties;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import com.fasterxml.jackson.core.util.DefaultPrettyPrinter;
import com.fasterxml.jackson.databind.ObjectMapper;

import rina.applicationprocess.api.WhatevercastName;
import rina.aux.LogHelper;
import rina.cdap.api.CDAPSessionManager;
import rina.cdap.impl.CDAPSessionManagerImpl;
import rina.cdap.impl.googleprotobuf.GoogleProtocolBufWireMessageProviderFactory;
import rina.configuration.RINAConfiguration;
import rina.delimiting.api.Delimiter;
import rina.delimiting.impl.DIFDelimiter;
import rina.encoding.api.Encoder;
import rina.encoding.impl.EncoderImpl;
import rina.encoding.impl.googleprotobuf.applicationregistration.ApplicationRegistrationEncoder;
import rina.encoding.impl.googleprotobuf.datatransferconstants.DataTransferConstantsEncoder;
import rina.encoding.impl.googleprotobuf.directoryforwardingtable.DirectoryForwardingTableEntryArrayEncoder;
import rina.encoding.impl.googleprotobuf.directoryforwardingtable.DirectoryForwardingTableEntryEncoder;
import rina.encoding.impl.googleprotobuf.enrollment.EnrollmentInformationEncoder;
import rina.encoding.impl.googleprotobuf.flow.FlowEncoder;
import rina.encoding.impl.googleprotobuf.flowservice.FlowServiceEncoder;
import rina.encoding.impl.googleprotobuf.neighbor.NeighborArrayEncoder;
import rina.encoding.impl.googleprotobuf.neighbor.NeighborEncoder;
import rina.encoding.impl.googleprotobuf.qoscube.QoSCubeArrayEncoder;
import rina.encoding.impl.googleprotobuf.qoscube.QoSCubeEncoder;
import rina.encoding.impl.googleprotobuf.whatevercast.WhatevercastNameArrayEncoder;
import rina.encoding.impl.googleprotobuf.whatevercast.WhatevercastNameEncoder;
import rina.enrollment.api.EnrollmentInformationRequest;
import rina.enrollment.api.EnrollmentTask;
import rina.flowallocator.api.DirectoryForwardingTableEntry;
import rina.flowallocator.api.Flow;
import rina.ipcprocess.impl.ecfp.DataTransferConstantsRIBObject;
import rina.ipcprocess.impl.enrollment.EnrollmentTaskImpl;
import rina.ipcprocess.impl.flowallocator.ribobjects.DirectoryForwardingTableEntrySetRIBObject;
import rina.ipcprocess.impl.flowallocator.ribobjects.QoSCubeSetRIBObject;
import rina.ipcprocess.impl.resourceallocator.ResourceAllocatorImpl;
import rina.ipcprocess.impl.ribdaemon.RIBDaemonImpl;
import rina.ipcprocess.impl.ribobjects.WhatevercastNameSetRIBObject;
import rina.resourceallocator.api.ResourceAllocator;
import rina.ribdaemon.api.RIBDaemon;
import rina.ribdaemon.api.RIBObject;

import eu.irati.librina.AllocateFlowRequestResultEvent;
import eu.irati.librina.ApplicationProcessNamingInformation;
import eu.irati.librina.ApplicationRegistration;
import eu.irati.librina.AssignToDIFRequestEvent;
import eu.irati.librina.AssignToDIFResponseEvent;
import eu.irati.librina.DIFInformation;
import eu.irati.librina.DataTransferConstants;
import eu.irati.librina.DeallocateFlowResponseEvent;
import eu.irati.librina.EnrollToDIFRequestEvent;
import eu.irati.librina.ExtendedIPCManagerSingleton;
import eu.irati.librina.FlowDeallocatedEvent;
import eu.irati.librina.FlowInformation;
import eu.irati.librina.FlowRequestEvent;
import eu.irati.librina.IPCEvent;
import eu.irati.librina.IPCEventProducerSingleton;
import eu.irati.librina.IPCEventType;
import eu.irati.librina.IPCProcessDIFRegistrationEvent;
import eu.irati.librina.KernelIPCProcessSingleton;
import eu.irati.librina.Neighbor;
import eu.irati.librina.QoSCube;
import eu.irati.librina.rina;

public class IPCProcess {
	
	public static final String MANAGEMENT_AE = "Management";
    public static final String DATA_TRANSFER_AE = "Data Transfer";
    public static final int DEFAULT_MAX_SDU_SIZE_IN_BYTES = 10000;
	public static final long CONFIG_FILE_POLL_PERIOD_IN_MS = 5000;
    
    public enum State {NULL, INITIALIZED, ASSIGN_TO_DIF_IN_PROCESS, ASSIGNED_TO_DIF};
	
	private static final Log log = LogFactory.getLog(IPCProcess.class);
	
	/** The state */
	private State operationalState = State.NULL;
	
	/** To get events from the Kernel IPC Process or the IPC Manager */
	private IPCEventProducerSingleton ipcEventProducer = null;
	
	/** To communicate with the IPC Manager */
	private ExtendedIPCManagerSingleton ipcManager = null;
	
	/** To communicate with the IPC Process components in the kernel */
	private KernelIPCProcessSingleton kernelIPCProcess = null;
	
	/** Map of events pending to be replied */
	private Map<Long, IPCEvent> pendingEvents = null;
	
	/** Information about the dif where the IPC Process is currently assigned */
	private DIFInformation difInformation = null;
	
	/** The DIF Delimiter */
	private Delimiter delimiter = null;
	
	/** The CDAP Session Manager */
	private CDAPSessionManager cdapSessionManager = null;
	
	/** The Encoder of objects, to that they can be carried as CDAP object values */
	private Encoder encoder = null;
	
	/** The RIB Daemon */
	private RIBDaemon ribDaemon = null;
	
	/** The enrollment task */
	private EnrollmentTask enrollmentTask = null;
	
	/** The Resource Allocator */
	private ResourceAllocator resourceAllocator = null;
	
	/** Static instance to implement the singleton pattern */
	private static IPCProcess instance = null;
	
	/** The IPC Process name */
	private ApplicationProcessNamingInformation name = null;
	
	/** The thread pool implementation */
	private ExecutorService executorService = null;
	
	/** The config file location */
	private String configFileLocation = null;
	
	public static IPCProcess getInstance() {
		if (instance == null) {
			instance = new IPCProcess();
		}
		
		return instance;
	}
	
	private IPCProcess(){
	}
	
	public void initialize(
			ApplicationProcessNamingInformation namingInfo, int id, long ipcManagerPort) throws Exception{
		log.info("Initializing configuration... ");
		executorService = Executors.newCachedThreadPool();
		initializeConfiguration();
		log.info("Initializing librina...");
		rina.initialize(LogHelper.getLibrinaLogLevel(), LogHelper.getLibrinaLogFile());
		ipcEventProducer = rina.getIpcEventProducer();
		kernelIPCProcess = rina.getKernelIPCProcess();
		kernelIPCProcess.setIPCProcessId(id);
		ipcManager = rina.getExtendedIPCManager();
		ipcManager.setIpcProcessId(id);
		ipcManager.setIPCManagerPort(ipcManagerPort);
		name = namingInfo;
		pendingEvents = new ConcurrentHashMap<Long, IPCEvent>();
		delimiter = new DIFDelimiter();
		cdapSessionManager = new CDAPSessionManagerImpl(
				new GoogleProtocolBufWireMessageProviderFactory());
		encoder = createEncoder();
		ribDaemon = new RIBDaemonImpl();
		enrollmentTask = new EnrollmentTaskImpl();
		resourceAllocator = new ResourceAllocatorImpl();
		((RIBDaemonImpl) ribDaemon).setIPCProcess(this);
		((EnrollmentTaskImpl) enrollmentTask).setIPCProcess(this);
		((ResourceAllocatorImpl) resourceAllocator).setIPCProcess(this);
		
		populateRIB();

		log.info("Initialized IPC Process with AP name: "+namingInfo.getProcessName()
				+ "; AP instance: "+namingInfo.getProcessInstance() + "; id: "+id);

		log.info("Notifying IPC Manager about successful initialization... ");
		try {
			ipcManager.notifyIPCProcessInitialized(name);
		} catch (Exception ex) {
			log.error("Problems notifying IPC Manager about successful initialization: " + ex.getMessage());
			log.error("Shutting down IPC Process");
			System.exit(-1);
		}
		setOperationalState(State.INITIALIZED);

		log.info("IPC Manager notified successfully");
	}
	
	private void initializeConfiguration(){
		
		Properties prop = new Properties(); 
		try {
			prop.load(this.getClass().getResourceAsStream("/ipcprocess.properties"));
			configFileLocation = prop.getProperty("configFileLocation");
			if (configFileLocation == null) {
				log.error("Could not find location of config file, exiting");
				System.exit(-1);
			}
		} 
		catch (Exception ex) {
			log.error("Could not find IPC Process properties file, exiting: "+ex.getMessage());
			System.exit(-1);
		}
		
		//Read config file
		RINAConfiguration rinaConfiguration = readConfigurationFile();
		RINAConfiguration.setConfiguration(rinaConfiguration);
		
		//Start thread that will look for config file changes
		Runnable configFileChangeRunnable = new Runnable(){
			private long currentLastModified = System.currentTimeMillis();
			private RINAConfiguration rinaConfiguration = null;

			public void run(){
				File file = new File(configFileLocation);

				while(true){
					if (file.lastModified() > currentLastModified) {
						log.debug("Configuration file changed, loading the new content");
						currentLastModified = file.lastModified();
						rinaConfiguration = readConfigurationFile();
						if (rinaConfiguration != null) {
							RINAConfiguration.setConfiguration(rinaConfiguration);
						};
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
    			objectMapper.readValue(new FileInputStream(configFileLocation), 
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
	
	public void execute(Runnable runnable) {
		executorService.execute(runnable);
	}
	
	public Delimiter getDelimiter() {
		return delimiter;
	}
	
	public CDAPSessionManager getCDAPSessionManager() {
		return cdapSessionManager;
	}
	
	public Encoder getEncoder() {
		return encoder;
	}
	
	public RIBDaemon getRIBDaemon() {
		return ribDaemon;
	}
	
	public EnrollmentTask getEnrollmentTask() {
		return enrollmentTask;
	}
	
	public ResourceAllocator getResourceAllocator() {
		return resourceAllocator;
	}
	
	private Encoder createEncoder() {
		  EncoderImpl encoder = new EncoderImpl();
          encoder.addEncoder(DataTransferConstants.class.getName(), new DataTransferConstantsEncoder());
          encoder.addEncoder(DirectoryForwardingTableEntry.class.getName(), new DirectoryForwardingTableEntryEncoder());
          encoder.addEncoder(DirectoryForwardingTableEntry[].class.getName(), new DirectoryForwardingTableEntryArrayEncoder());
          encoder.addEncoder(EnrollmentInformationRequest.class.getName(), new EnrollmentInformationEncoder());
          encoder.addEncoder(Flow.class.getName(), new FlowEncoder());
          encoder.addEncoder(QoSCube.class.getName(), new QoSCubeEncoder());
          encoder.addEncoder(QoSCube[].class.getName(), new QoSCubeArrayEncoder());
          encoder.addEncoder(WhatevercastName.class.getName(), new WhatevercastNameEncoder());
          encoder.addEncoder(WhatevercastName[].class.getName(), new WhatevercastNameArrayEncoder());
          encoder.addEncoder(FlowInformation.class.getName(), new FlowServiceEncoder());
          encoder.addEncoder(ApplicationRegistration.class.getName(), new ApplicationRegistrationEncoder());
          encoder.addEncoder(Neighbor.class.getName(), new NeighborEncoder());
          encoder.addEncoder(Neighbor[].class.getName(), new NeighborArrayEncoder());
          
          return encoder;
	}
	
	private void populateRIB(){
		try {
			RIBObject ribObject = new QoSCubeSetRIBObject();
			this.ribDaemon.addRIBObject(ribObject);
			ribObject = new WhatevercastNameSetRIBObject();
			this.ribDaemon.addRIBObject(ribObject);
			ribObject = new DataTransferConstantsRIBObject();
			this.ribDaemon.addRIBObject(ribObject);
			ribObject = new DirectoryForwardingTableEntrySetRIBObject();
			this.ribDaemon.addRIBObject(ribObject);
		} catch(Exception ex) {
			log.error("Problems populating RIB: "+ex.getMessage());
		}
	}
	
	public State getOperationalState(){
        return operationalState;
	}
	
	public void setOperationalState(State state) {
		this.operationalState = state;
	}
	
	public ApplicationProcessNamingInformation getName() {
		return name;
	}
	
	public void executeEventLoop(){
		IPCEvent event = null;
		
		while(true){
			try{
				event = ipcEventProducer.eventWait();
				processEvent(event);
			}catch(Exception ex){
				ex.printStackTrace();
				log.error("Problems processing event of type " + event.getType() + 
						". " + ex.getMessage());
			}
		}
	}
	
	private void processEvent(IPCEvent event) throws Exception{
		log.info("Got event of type: "+event.getType() 
				+ " and sequence number: "+event.getSequenceNumber());
		
		if (event.getType() == IPCEventType.IPC_PROCESS_DIF_REGISTRATION_NOTIFICATION) {
			IPCProcessDIFRegistrationEvent regEvent = (IPCProcessDIFRegistrationEvent) event;
			resourceAllocator.getNMinus1FlowManager().processRegistrationNotification(regEvent);
		} else if (event.getType() == IPCEventType.ASSIGN_TO_DIF_REQUEST_EVENT) {
			AssignToDIFRequestEvent asEvent = (AssignToDIFRequestEvent) event;
			processAssignToDIFRequest(asEvent);
		} else if (event.getType() == IPCEventType.ASSIGN_TO_DIF_RESPONSE_EVENT) {
			AssignToDIFResponseEvent asEvent = (AssignToDIFResponseEvent) event;
			processAssignToDIFResponseEvent(asEvent);
		} else if (event.getType() == IPCEventType.ALLOCATE_FLOW_REQUEST_RESULT_EVENT) {
			AllocateFlowRequestResultEvent flowEvent = (AllocateFlowRequestResultEvent) event;
			resourceAllocator.getNMinus1FlowManager().allocateRequestResult(flowEvent);
		} else if (event.getType() == IPCEventType.FLOW_ALLOCATION_REQUESTED_EVENT) {
			FlowRequestEvent flowRequestEvent = (FlowRequestEvent) event;
			resourceAllocator.getNMinus1FlowManager().flowAllocationRequested(flowRequestEvent);
		} else if (event.getType() == IPCEventType.DEALLOCATE_FLOW_RESPONSE_EVENT) {
			DeallocateFlowResponseEvent flowEvent = (DeallocateFlowResponseEvent) event;
			resourceAllocator.getNMinus1FlowManager().deallocateFlowResponse(flowEvent);
		} else if (event.getType() == IPCEventType.FLOW_DEALLOCATED_EVENT) {
			FlowDeallocatedEvent flowEvent = (FlowDeallocatedEvent) event;
			resourceAllocator.getNMinus1FlowManager().flowDeallocatedRemotely(flowEvent);
		} else if (event.getType() == IPCEventType.ENROLL_TO_DIF_REQUEST_EVENT) {
			EnrollToDIFRequestEvent enrEvent = (EnrollToDIFRequestEvent) event;
			enrollmentTask.processEnrollmentRequestEvent(enrEvent, difInformation);
		}
	}
	
	private synchronized void processAssignToDIFRequest(AssignToDIFRequestEvent event) throws Exception{
		if (getOperationalState() != State.INITIALIZED) {
			//The IPC Process can only be assigned to a DIF once, reply with error message
			log.error("Got a DIF assignment request while not in INITIALIZED state. " 
						+ "Current state is: " + getOperationalState());
			ipcManager.assignToDIFResponse(event, -1);
			return;
		}
		
		try {
			long handle = kernelIPCProcess.assignToDIF(event.getDIFInformation());
			pendingEvents.put(handle, event);
			setOperationalState(State.ASSIGN_TO_DIF_IN_PROCESS);
		} catch (Exception ex) {
			log.error("Problems sending DIF Assignment request to the kernel: "+ex.getMessage());
			ipcManager.assignToDIFResponse(event, -1);
		}
	}
	
	private synchronized void processAssignToDIFResponseEvent(AssignToDIFResponseEvent event) throws Exception{
		if (getOperationalState() != State.ASSIGN_TO_DIF_IN_PROCESS) {
			log.error("Got a DIF assignment response while not in ASSIGN_TO_DIF_IN_CPORCES state." 
						+ " Current state is " + getOperationalState());
			return;
		}
		
		IPCEvent ipcEvent = pendingEvents.remove(event.getSequenceNumber());
		if (ipcEvent == null) {
			log.error("Couldn't find an Assign to DIF Request Event associated to the handle " + 
						event.getSequenceNumber());
			return;
		}
		
		if (!(ipcEvent instanceof AssignToDIFRequestEvent)){
			log.error("Expected an Assign To DIF Request Event, but got event of type: " + ipcEvent.getClass().getName());
			return;
		}
		
		AssignToDIFRequestEvent arEvent = (AssignToDIFRequestEvent) ipcEvent;
		if (event.getResult() == 0) {
			log.debug("The kernel processed successfully the Assign to DIF Request");
			ipcManager.assignToDIFResponse(arEvent, 0);
			setOperationalState(State.ASSIGNED_TO_DIF);
			difInformation = arEvent.getDIFInformation();
			
			if (difInformation.getDifConfiguration().getQosCubes().size() > 0) {
				QoSCube[] qosCubes = new QoSCube[(int)difInformation.getDifConfiguration().getQosCubes().size()];
				Iterator<QoSCube> iterator = difInformation.getDifConfiguration().getQosCubes().iterator();
				int i = 0;
				while(iterator.hasNext()) {
					qosCubes[i] = iterator.next();
					i++;
				}
				
				ribDaemon.create(QoSCubeSetRIBObject.QOSCUBE_RIB_OBJECT_CLASS, 
						QoSCubeSetRIBObject.QOSCUBE_SET_RIB_OBJECT_NAME, qosCubes);
			}
			
			log.info("IPC Process successfully assigned to DIF "+ difInformation.getDifName());
		} else {
			log.error("The kernel couldn't successfully process the Assign to DIF Request: "+ event.getResult());
			ipcManager.assignToDIFResponse(arEvent, -1);
			setOperationalState(State.INITIALIZED);
			log.error("Could not assign IPC Process to DIF " + difInformation.getDifName());
		}
	}
	
	public synchronized DIFInformation getDIFInformation() {
		return difInformation;
	}
	
	public synchronized void setDIFInformation(DIFInformation difInformation) {
		this.difInformation = difInformation;
	}
	
	public synchronized Long getAddress() {
		if (difInformation == null) {
			return null;
		}
		
		return new Long(difInformation.getDifConfiguration().getAddress());
	}
	
	/**
     * Returns the list of IPC processes that are part of the DIF this IPC Process is part of
     * @return
     */
    public List<Neighbor> getNeighbors(){
            return enrollmentTask.getNeighbors();
    }

}
