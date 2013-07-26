package rina.ipcmanager.impl;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import com.fasterxml.jackson.core.util.DefaultPrettyPrinter;
import com.fasterxml.jackson.databind.ObjectMapper;

import rina.applicationprocess.api.WhatevercastName;
import rina.cdap.api.CDAPSessionManagerFactory;
import rina.ipcmanager.impl.conf.DIFConfiguration;
import rina.ipcmanager.impl.conf.IPCProcessToCreate;
import rina.ipcmanager.impl.conf.KnownIPCProcessAddress;
import rina.ipcmanager.impl.conf.RINAConfiguration;
import rina.ipcmanager.impl.conf.SDUProtectionOption;
import rina.delimiting.api.DelimiterFactory;
import rina.efcp.api.DataTransferConstants;
import rina.encoding.api.EncoderFactory;
import rina.enrollment.api.BaseEnrollmentTask;
import rina.enrollment.api.EnrollmentTask;
import rina.enrollment.api.Neighbor;
import rina.flowallocator.api.FlowAllocatorInstance;
import rina.flowallocator.api.QoSCube;
import rina.flowallocator.api.Flow;
import rina.idd.api.InterDIFDirectoryFactory;
import rina.ipcmanager.api.IPCManager;
import rina.ipcmanager.impl.apservice.APServiceTCPServer;
import rina.ipcmanager.impl.apservice.SDUDeliveryService;
import rina.ipcmanager.impl.console.IPCManagerConsole;
import rina.ipcprocess.api.IPCProcess;
import rina.ipcprocess.api.IPCProcessFactory;
import rina.applicationprocess.api.ApplicationProcessNamingInfo;
import rina.aux.BlockingQueueWithSubscriptor;
import rina.ipcservice.api.IPCException;
import rina.ipcservice.api.IPCService;
import rina.resourceallocator.api.BaseResourceAllocator;
import rina.resourceallocator.api.ResourceAllocator;
import rina.ribdaemon.api.BaseRIBDaemon;
import rina.ribdaemon.api.RIBDaemon;
import rina.ribdaemon.api.RIBObject;
import rina.ribdaemon.api.RIBObjectNames;

/**
 * The IPC Manager is the component of a DAF that manages the local IPC resources. In its current implementation it 
 * manages IPC Processes (creates/destroys them), and serves as a broker between applications and IPC Processes. Applications 
 * can use the RINA library to establish a connection to the IPC Manager and interact with the RINA stack.
 * @author eduardgrasa
 *
 */
public class IPCManagerImpl implements IPCManager{
	
	private static final Log log = LogFactory.getLog(IPCManagerImpl.class);
	
	public static final String CONFIG_FILE_LOCATION = "config/config.rina"; 
	public static final long CONFIG_FILE_POLL_PERIOD_IN_MS = 5000;
	
	private IPCManagerConsole console = null;
	
	/**
	 * The thread pool implementation
	 */
	private ExecutorService executorService = null;
	
	/**
	 * The IPC Process factories
	 */
	private Map<String, IPCProcessFactory> ipcProcessFactories = null;
	
	private APServiceTCPServer apServiceTCPServer = null;
	
	/**
	 * The list that contains the portIds that are currently in use
	 */
	private List<Integer> portIdsInUse = null;
	
	/**
	 * The incoming flow queues of all the IPC Processes in this instantiation 
	 * of the RINA software
	 */
	private Map<Integer, BlockingQueueWithSubscriptor<byte[]>> incomingFlowQueues = null;
	
	/**
	 * The outgoing flow queues of all the IPC Processes in this instantiation 
	 * of the RINA software
	 */
	private Map<Integer, BlockingQueueWithSubscriptor<byte[]>> outgoingFlowQueues = null;
	
	/**
	 * the SDU Delivery Service
	 */
	private SDUDeliveryService sduDeliveryService = null;
	
	public IPCManagerImpl(){
		this.ipcProcessFactories = new HashMap<String, IPCProcessFactory>();
		executorService = Executors.newCachedThreadPool();
		initializeConfiguration();
		console = new IPCManagerConsole(this);
		executorService.execute(console);
		sduDeliveryService = new SDUDeliveryService(this);
		executorService.execute(sduDeliveryService);
		apServiceTCPServer = new APServiceTCPServer(this, sduDeliveryService);
		executorService.execute(apServiceTCPServer);
		this.portIdsInUse = new ArrayList<Integer>();
		this.incomingFlowQueues = new ConcurrentHashMap<Integer, BlockingQueueWithSubscriptor<byte[]>>();
		this.outgoingFlowQueues = new ConcurrentHashMap<Integer, BlockingQueueWithSubscriptor<byte[]>>();
		log.debug("IPC Manager started");
	}
	
	private void initializeConfiguration(){
		//Read config file
		RINAConfiguration rinaConfiguration = readConfigurationFile();
		RINAConfiguration.setConfiguration(rinaConfiguration);
		
		//Start thread that will look for config file changes
		Runnable configFileChangeRunnable = new Runnable(){
			private long currentLastModified = 0;
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
	 * Called by Spring DM every time an IPC Process factory is registered
	 * @param serviceInstance
	 * @param serviceProperties
	 */
	public void ipcProcessFactoryAdded(IPCProcessFactory serviceInstance, Map serviceProperties){
		if (serviceInstance != null && serviceProperties != null){
			serviceInstance.setIPCManager(this);
			ipcProcessFactories.put((String)serviceProperties.get("type"), serviceInstance);
			log.debug("New IPC Process factory added for IPC Processes of type: "+serviceProperties.get("type"));
		}
	}
	
	/**
	 * Called by Spring DM every time an IPC Process factory is unregistered
	 * @param serviceInstance
	 * @param serviceProperties
	 */
	public void ipcProcessFactoryRemoved(IPCProcessFactory serviceInstance, Map serviceProperties){
		if (serviceInstance != null && serviceProperties != null){
			ipcProcessFactories.remove((String)serviceProperties.get("type"));
			log.debug("Existing IPC Process factory removed for IPC Processes of type: "+serviceProperties.get("type"));
		}
	}
	
	/**
	 * Executes a runnable in a thread. The IPCManager maintains a single thread pool 
	 * for all the RINA prototype
	 * @param runnable
	 */
	public synchronized void execute(Runnable runnable){
		executorService.execute(runnable);
	}
	
	public void stop(){
		apServiceTCPServer.setEnd(true);
		console.stop();
		executorService.shutdownNow();
	}
	
	public void setInterDIFDirectoryFactory(InterDIFDirectoryFactory iddFactory){
		apServiceTCPServer.setInterDIFDirectory(iddFactory.createIDD(this));
	}
	
	/**
	 * Create the initial IPC Processes as specified in the configuration file (if any)
	 */
	public void createInitialProcesses(){
		RINAConfiguration rinaConfiguration = RINAConfiguration.getInstance();
		if (rinaConfiguration.getIpcProcessesToCreate() == null){
			return;
		}
		
		IPCProcessToCreate currentProcess = null;
		for(int i=0; i<rinaConfiguration.getIpcProcessesToCreate().size(); i++){
			currentProcess = rinaConfiguration.getIpcProcessesToCreate().get(i);
			try{
				this.createIPCProcess(currentProcess.getType(), currentProcess.getApplicationProcessName(), 
						currentProcess.getApplicationProcessInstance(), currentProcess.getDifName(), 
						rinaConfiguration, currentProcess.getNeighbors(), currentProcess.getDifsToRegisterAt(), 
						currentProcess.getSduProtectionOptions());
			}catch(Exception ex){
				ex.printStackTrace();
				log.error(ex);
			}
		}
	}

	public void createIPCProcess(String type, String apName, String apInstance, String difName, 
			RINAConfiguration config, List<Neighbor> neighbors, List<String> difsToRegisterAt, 
			List<SDUProtectionOption> sduProtectionOptions) throws Exception{
		IPCProcessFactory ipcProcessFactory = this.ipcProcessFactories.get(type);
		if (ipcProcessFactory == null){
			throw new Exception("Unsupported IPC Process type: "+type);
		}
		
		IPCProcess ipcProcess = ipcProcessFactory.createIPCProcess(apName, apInstance, config);
		ipcProcess.setIPCManager(this);
		
		if (type.equals(IPCProcessFactory.NORMAL)){
			if (difName != null){
				DIFConfiguration difConfiguration = (DIFConfiguration) RINAConfiguration.getInstance().getDIFConfiguration(difName);
				if (difConfiguration == null){
					throw new Exception("Unrecognized DIF name: "+difName);
				}

				KnownIPCProcessAddress ipcProcessAddress = 
					RINAConfiguration.getInstance().getIPCProcessAddress(difName, apName, apInstance);
				if (ipcProcessAddress == null){
					throw new Exception("Unrecoginzed IPC Process Name; "+apName+": "+apInstance);
				}

				WhatevercastName dan = new WhatevercastName();
				dan.setName(difName);
				dan.setRule(WhatevercastName.DIF_NAME_WHATEVERCAST_RULE);

				RIBDaemon ribDaemon = (RIBDaemon) ipcProcess.getIPCProcessComponent(BaseRIBDaemon.getComponentName());
				ribDaemon.create(WhatevercastName.WHATEVERCAST_NAME_RIB_OBJECT_CLASS, 
						WhatevercastName.WHATEVERCAST_NAME_SET_RIB_OBJECT_NAME + RIBObjectNames.SEPARATOR + 
						WhatevercastName.DIF_NAME_WHATEVERCAST_RULE, 
						dan);

				ribDaemon.write(RIBObjectNames.ADDRESS_RIB_OBJECT_CLASS, 
						RIBObjectNames.ADDRESS_RIB_OBJECT_NAME, 
						ipcProcessAddress.getAddress());

				ribDaemon.write(DataTransferConstants.DATA_TRANSFER_CONSTANTS_RIB_OBJECT_CLASS, 
						DataTransferConstants.DATA_TRANSFER_CONSTANTS_RIB_OBJECT_NAME, 
						difConfiguration.getDataTransferConstants());

				ribDaemon.create(QoSCube.QOSCUBE_SET_RIB_OBJECT_CLASS,
						QoSCube.QOSCUBE_SET_RIB_OBJECT_NAME, 
						(QoSCube[]) difConfiguration.getQosCubes().toArray(new QoSCube[difConfiguration.getQosCubes().size()]));

				if (neighbors != null){
					ribDaemon.create(Neighbor.NEIGHBOR_SET_RIB_OBJECT_CLASS, 
							Neighbor.NEIGHBOR_SET_RIB_OBJECT_NAME, 
							(Neighbor[]) neighbors.toArray(new Neighbor[neighbors.size()]));
				}

				ribDaemon.start(RIBObjectNames.OPERATIONAL_STATUS_RIB_OBJECT_CLASS, 
						RIBObjectNames.OPERATIONAL_STATUS_RIB_OBJECT_NAME);
			}
			
			ResourceAllocator resourceAllocator = null;
			if (difsToRegisterAt != null){
				resourceAllocator = (ResourceAllocator) ipcProcess.getIPCProcessComponent(BaseResourceAllocator.getComponentName());
				for(int i=0; i<difsToRegisterAt.size(); i++){
					try{
						resourceAllocator.getNMinus1FlowManager().registerIPCProcess(difsToRegisterAt.get(i));
					}catch(IPCException ex){
						log.error("Error registering IPC Process "+apName+" "+apInstance+" to N-1 DIF "+difsToRegisterAt);
					}
				}
			}
			
			if (sduProtectionOptions != null){
				resourceAllocator = (ResourceAllocator) ipcProcess.getIPCProcessComponent(BaseResourceAllocator.getComponentName());
				resourceAllocator.getNMinus1FlowManager().setSDUProtecionOptions(sduProtectionOptions);
			}
		}
	}
	
	public IPCProcess getIPCProcess(String apName, String apInstance) throws Exception{
		Iterator<IPCProcessFactory> iterator = this.ipcProcessFactories.values().iterator();
		IPCProcess ipcProcess = null;
		
		while(iterator.hasNext()){
			ipcProcess = iterator.next().getIPCProcess(apName, apInstance);
			if (ipcProcess != null){
				return ipcProcess;
			}
		}
		
		throw new Exception("Could not find IPC Process with AP name: "+apName+" and AP instance: "+apInstance);
	}
	
	public void destroyIPCProcesses(String apName, String apInstance) throws Exception{
		Iterator<IPCProcessFactory> iterator = this.ipcProcessFactories.values().iterator();
		IPCProcess ipcProcess = null;
		IPCProcessFactory ipcProcessFactory = null;
		
		while(iterator.hasNext()){
			ipcProcessFactory = iterator.next();
			ipcProcess = ipcProcessFactory.getIPCProcess(apName, apInstance);
			if (ipcProcess != null){
				ipcProcessFactory.destroyIPCProcess(ipcProcess);
				return;
			}
		}
		
		throw new Exception("Could not find IPC Process with AP name: "+apName+" and AP instance: "+apInstance);
	}
	
	/**
	 * Get a portId available for its use
	 * @return a positive portId if there's one available, 
	 * -1 if no free portIds are found
	 */
	public int getAvailablePortId(){
		Integer candidate = null;
		
		synchronized(this.portIdsInUse){
			for(int i=1; i<Integer.MAX_VALUE; i++){
				candidate = new Integer(i);
				if (this.portIdsInUse.contains(candidate)){
					continue;
				}

				this.portIdsInUse.add(candidate);
				return i;
			}

			return -1;
		}
	}
	
	/**
	 * Mark a portId as available to be reused
	 * @param portId
	 */
	public void freePortId(int portId){
		synchronized(this.portIdsInUse){
			this.portIdsInUse.remove(new Integer(portId));
		}
	}
	
	/**
	 * Add an incoming and outgoing flow queues to support the flow identified by portId
	 * @param portId
	  * @param queueCapacity the capacity of the queue, it it is <= 0 an unlimited capacity queue will be used
	 * @throws IPCException if the portId is already in use
	 */
	public void addFlowQueues(int portId, int queueCapacity) throws IPCException{
		Integer queueId = new Integer(portId);
		if (this.incomingFlowQueues.get(queueId) != null || 
				this.outgoingFlowQueues.get(queueId) != null){
			throw new IPCException(IPCException.PROBLEMS_ALLOCATING_FLOW_CODE, 
					IPCException.PROBLEMS_ALLOCATING_FLOW + ". There are existing queues supporting this portId");
		}
		
		this.incomingFlowQueues.put(queueId, new BlockingQueueWithSubscriptor<byte[]>(portId, queueCapacity, true));
		this.outgoingFlowQueues.put(queueId, new BlockingQueueWithSubscriptor<byte[]>(portId, queueCapacity, false));
	}
	
	/**
	 * Remove the incoming and outgoing flow queues that support the flow identified by portId
	 * @param portId
	 */
	public void removeFlowQueues(int portId){
		Integer queueId = new Integer(portId);
		this.incomingFlowQueues.remove(queueId);
		this.outgoingFlowQueues.remove(queueId);
	}
	
	/**
	 * Get the incoming queue that supports the flow identified by portId
	 * @param portId
	 * @return
	 * @throws IPCException if there is no incoming queue associated to portId
	 */
	public BlockingQueueWithSubscriptor<byte[]> getIncomingFlowQueue(int portId) throws IPCException{
		BlockingQueueWithSubscriptor<byte[]> result = this.incomingFlowQueues.get(new Integer(portId));
		if (result == null){
			throw new IPCException(IPCException.ERROR_CODE, "Could not find the incoming flow queue associated to portId "+portId);
		}
		
		return result;
	}
	
	/**
	 * Get the outgoing queue that supports the flow identified by portId
	 * @param portId
	 * @return
	 * @throws IPCException if there is no outgoing queue associated to portId
	 */
	public BlockingQueueWithSubscriptor<byte[]> getOutgoingFlowQueue(int portId) throws IPCException{
		BlockingQueueWithSubscriptor<byte[]> result = this.outgoingFlowQueues.get(new Integer(portId));
		if (result == null){
			throw new IPCException(IPCException.ERROR_CODE, "Could not find the outgoing flow queue associated to portId "+portId);
		}
		
		return result;
	}
	
	public String listIPCProcessesInformation(){
		Iterator<Entry<String, IPCProcessFactory>> iterator = this.ipcProcessFactories.entrySet().iterator();
		Entry<String, IPCProcessFactory> entry = null;
		String information = "";
		List<IPCProcess> ipcProcesses = null;
		
		while (iterator.hasNext()){
			entry = iterator.next();
			information = information + "\n\n*** Listing IPC Processes of type "+ entry.getKey() + " ***\n";
			ipcProcesses = entry.getValue().listIPCProcesses();
			ApplicationProcessNamingInfo apNamingInfo = null;
			String difName = null;
			
			for(int i=0; i<ipcProcesses.size(); i++){
				apNamingInfo = ipcProcesses.get(i).getApplicationProcessNamingInfo();
				difName = ipcProcesses.get(i).getDIFName();
				information = information + "\n";
				information = information + "DIF name: " + difName + "\n";
				information = information + "Application process name: "+apNamingInfo.getApplicationProcessName() + "\n";
				information = information + "Application process instance: "+apNamingInfo.getApplicationProcessInstance() + "\n";
				information = information + "Address: "+ipcProcesses.get(i).getAddress().longValue() + "\n";
			}
		}

		return information;
	}

	public List<String> getPrintedRIB(String apName, String apInstance) throws Exception{
		IPCProcess ipcProcess = getIPCProcess(apName, apInstance);
		RIBDaemon ribDaemon = (RIBDaemon) ipcProcess.getIPCProcessComponent(BaseRIBDaemon.getComponentName());
		List<RIBObject> ribObjects = ribDaemon.getRIBObjects();
		List<String> result = new ArrayList<String>();
		RIBObject currentRIBObject = null;
		String object = null;
		
		for(int i=0; i<ribObjects.size(); i++){
			currentRIBObject = ribObjects.get(i);
			object = "\nObject name: " + currentRIBObject.getObjectName() + "\n";
			object = object + "Object class: " + currentRIBObject.getObjectClass() + "\n";
			object = object + "Object instance: " + currentRIBObject.getObjectInstance() + "\n";
			object = object + "Object value: " + currentRIBObject.getObjectValue() + "\n";
			result.add(object);
		}
		
		return result;
	}
	
	public void enroll(String sourceAPName, String sourceAPInstance, String destAPName, String destAPInstance) throws Exception{
		IPCProcess ipcProcess = getIPCProcess(sourceAPName, sourceAPInstance);
		EnrollmentTask enrollmentTask = (EnrollmentTask) ipcProcess.getIPCProcessComponent(BaseEnrollmentTask.getComponentName());
		
		Neighbor neighbor = new Neighbor();
		neighbor.setApplicationProcessName(destAPName);
		neighbor.setApplicationProcessInstance(destAPInstance);

		enrollmentTask.initiateEnrollment(neighbor);
	}
	
	public void writeDataToFlow(String sourceAPName, String sourceAPInstance, int portId, String data) throws Exception{
		IPCService ipcService = (IPCService) getIPCProcess(sourceAPName, sourceAPInstance);
		ipcService.submitTransfer(portId, data.getBytes());
	}
	
	public void deallocateFlow(String sourceAPName, String sourceAPInstance, int portId) throws Exception{
		IPCProcess ipcProcess = getIPCProcess(sourceAPName, sourceAPInstance);
		IPCService ipcService = (IPCService) ipcProcess;
		ipcService.submitDeallocate(portId);
	}

	public void createFlowRequestMessageReceived(Flow arg0, FlowAllocatorInstance arg1) {
	}

	public CDAPSessionManagerFactory getCDAPSessionManagerFactory(){
		return this.ipcProcessFactories.get(IPCProcessFactory.NORMAL).getCDAPSessionManagerFactory();
	}
	
	public DelimiterFactory getDelimiterFactory(){
		return this.ipcProcessFactories.get(IPCProcessFactory.NORMAL).getDelimiterFactory();
	}
	
	public EncoderFactory getEncoderFactory(){
		return this.ipcProcessFactories.get(IPCProcessFactory.NORMAL).getEncoderFactory();
	}
	
	public List<IPCProcess> listIPCProcesses(){
		Iterator<IPCProcessFactory> iterator = this.ipcProcessFactories.values().iterator();
		IPCProcessFactory ipcProcessFactory = null;
		List<IPCProcess> result = new ArrayList<IPCProcess>();
		
		while(iterator.hasNext()){
			ipcProcessFactory = iterator.next();
			result.addAll(ipcProcessFactory.listIPCProcesses());
		}
		
		return result;
	}
	
	public List<String> listDIFNames(){
		Iterator<IPCProcessFactory> iterator = this.ipcProcessFactories.values().iterator();
		IPCProcessFactory ipcProcessFactory = null;
		List<String> result = new ArrayList<String>();
		
		while(iterator.hasNext()){
			ipcProcessFactory = iterator.next();
			result.addAll(ipcProcessFactory.listDIFNames());
		}
		
		return result;
	}
	
	public IPCProcess getIPCProcessBelongingToDIF(String difName){
		Iterator<IPCProcessFactory> iterator = this.ipcProcessFactories.values().iterator();
		IPCProcessFactory ipcProcessFactory = null;
		IPCProcess result = null;
		
		while(iterator.hasNext()){
			ipcProcessFactory = iterator.next();
			result = ipcProcessFactory.getIPCProcessBelongingToDIF(difName);
			if (result != null){
				return result;
			}
		}
		
		return null;
	}
}
