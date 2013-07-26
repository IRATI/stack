package rina.ipcmanager.impl.test;

import java.net.Socket;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

import junit.framework.Assert;

import org.junit.Before;
import org.junit.Test;

import rina.cdap.api.CDAPSessionManager;
import rina.cdap.api.message.CDAPMessage;
import rina.cdap.api.message.CDAPMessage.Opcode;
import rina.cdap.api.message.ObjectValue;
import rina.delimiting.api.Delimiter;
import rina.delimiting.api.DelimiterFactory;
import rina.encoding.api.Encoder;
import rina.idd.api.InterDIFDirectoryFactory;
import rina.ipcmanager.impl.IPCManagerImpl;
import rina.ipcmanager.impl.apservice.APServiceTCPServer;
import rina.ipcprocess.api.IPCProcessFactory;
import rina.applicationprocess.api.ApplicationProcessNamingInfo;
import rina.ipcservice.api.ApplicationRegistration;
import rina.ipcservice.api.FlowService;

public class IPCManagerAppInteractionTest {
	
	private IPCManagerImpl ipcManager = null;
	private IPCProcessFactory mockIPCProcessFactory = null;
	private CDAPSessionManager cdapSessionManager = null;
	private Delimiter delimiter = null;
	private Encoder encoder = null;
	private InterDIFDirectoryFactory iddFactory = null;
	private ExecutorService executorService = null;
	
	@Before
	public void setup(){
		ipcManager = new IPCManagerImpl();
		mockIPCProcessFactory = new MockIPCProcessFactory();
		Map<String, String> serviceProperties = new HashMap<String, String>();
		serviceProperties.put("type", "normal");
		ipcManager.ipcProcessFactoryAdded(mockIPCProcessFactory, serviceProperties);
		iddFactory = new MockInterDIFDirectoryFactory();
		ipcManager.setInterDIFDirectoryFactory(iddFactory);
		cdapSessionManager = mockIPCProcessFactory.getCDAPSessionManagerFactory().createCDAPSessionManager();
		delimiter = mockIPCProcessFactory.getDelimiterFactory().createDelimiter(DelimiterFactory.DIF);
		encoder = mockIPCProcessFactory.getEncoderFactory().createEncoderInstance();
		executorService = Executors.newFixedThreadPool(10);
	}
	
	@Test
	public void testClientBehaviourFlowAllocateWriteReadDeallocateAllOk(){
		/*try{
			//1 Connect to the IPC Manager
			Socket rinaLibrarySocket = new Socket("localhost", APServiceTCPServer.DEFAULT_PORT);
			Assert.assertTrue(rinaLibrarySocket.isConnected());
			
			//2 Start a thread that will continuously listen to the socket waiting for IPC Manager responses
			SocketReader socketReader = new SocketReader(rinaLibrarySocket, delimiter, cdapSessionManager, this);
			executorService.execute(socketReader);
			
			//3 Get an allocate object and send the request to the IPC Manager
			CDAPMessage cdapMessage = getAllocateRequest();
			sendCDAPMessage(rinaLibrarySocket, cdapMessage);
			
			//4 Wait a bit and check that we got the reply
			wait2Seconds();
			cdapMessage = socketReader.getLastMessage();
			Assert.assertNotNull(cdapMessage);
			Assert.assertEquals(cdapMessage.getResult(), 0);
			Assert.assertEquals(cdapMessage.getOpCode(), Opcode.M_CREATE_R);
			byte[] encodedValue = cdapMessage.getObjValue().getByteval();
			FlowService flowService = (FlowService) encoder.decode(encodedValue, FlowService.class);
			System.out.println("Allocated portId: "+flowService.getPortId());
			
			//5 Write data
			cdapMessage = getWriteDataRequest("The rain in Spain stays mainly in the plain".getBytes());
			sendCDAPMessage(rinaLibrarySocket, cdapMessage);
			
			//6 Wait a bit and Read data
			wait2Seconds();
			cdapMessage = socketReader.getLastMessage();
			Assert.assertNotNull(cdapMessage);
			Assert.assertEquals(cdapMessage.getResult(), 0);
			Assert.assertEquals(cdapMessage.getOpCode(), Opcode.M_WRITE);
			byte[] sdu = cdapMessage.getObjValue().getByteval();
			String receivedMessage = new String(sdu);
			System.out.println("Received message: "+receivedMessage);
			Assert.assertEquals(receivedMessage, "In Hertford, Hereford, and Hampshire, hurricanes hardly ever happen");
			
			//7 Request to unallocate the flow
			cdapMessage = getDeallocateRequest();
			sendCDAPMessage(rinaLibrarySocket, cdapMessage);
			
			//8 Wait a bit and stop
			wait2Seconds();
			System.out.println("Flow deallocated");
			ipcManager.stop();
		}catch(Exception ex){
			ex.printStackTrace();
		}*/
	}
	
	@Test
	public void testServerBehaviourRegisterAllocateDeallocateUnregisterAllOk(){
		/*try{
			//1 Connect to the IPC Manager
			Socket rinaLibrarySocket = new Socket("localhost", APServiceTCPServer.DEFAULT_PORT);
			Assert.assertTrue(rinaLibrarySocket.isConnected());
			
			//2 Start a thread that will continuously listen to the socket waiting for IPC Manager responses
			SocketReader socketReader = new SocketReader(rinaLibrarySocket, delimiter, cdapSessionManager, this);
			executorService.execute(socketReader);
			
			//3 Start the TCPServer where the application will be listening for incoming requests, and issue the 
			// registration call
			TCPServer tcpServer = new TCPServer(delimiter, cdapSessionManager, encoder);
			executorService.execute(tcpServer);
			ApplicationRegistration applicationRegistration = new ApplicationRegistration();
			applicationRegistration.setApNamingInfo(new ApplicationProcessNamingInfo("A", "1"));
			applicationRegistration.setSocketNumber(tcpServer.getPort());
			CDAPMessage cdapMessage = getRegistrationRequest(applicationRegistration);
			sendCDAPMessage(rinaLibrarySocket, cdapMessage);
			
			//4 Wait a bit and check that we have the response
			wait2Seconds();
			cdapMessage = socketReader.getLastMessage();
			Assert.assertNotNull(cdapMessage);
			Assert.assertEquals(cdapMessage.getResult(), 0);
			Assert.assertEquals(cdapMessage.getOpCode(), Opcode.M_START_R);
			System.out.println("Application registered");
			
			//5 Allocation request arrives to AP service
			FlowService flowService = new FlowService();
			flowService.setDestinationAPNamingInfo(new ApplicationProcessNamingInfo("A", "1"));
			flowService.setSourceAPNamingInfo(new ApplicationProcessNamingInfo("B", "1"));
			flowService.setPortId(24);
			MockIPCProcess ipcService = (MockIPCProcess) mockIPCProcessFactory.createIPCProcess(null, null, null);
			ipcService.setFlowService(flowService);
			//String result = ipcManager.getAPService().deliverAllocateRequest(flowService, (IPCService) ipcService);
			//Assert.assertNull(result);
			wait2Seconds();
			
			//6 Deallocation request arrives to AP service
			//ipcManager.getAPService().deliverDeallocate(flowService.getPortId());
			wait2Seconds();
			
			//7 Unregister application
			cdapMessage = CDAPMessage.getStopObjectRequestMessage(null, null, null, null, 0, null, 0);
			sendCDAPMessage(rinaLibrarySocket, cdapMessage);
			
			//8 Wait a bit and check that we have the response
			wait2Seconds();
			cdapMessage = socketReader.getLastMessage();
			Assert.assertNotNull(cdapMessage);
			Assert.assertEquals(cdapMessage.getResult(), 0);
			Assert.assertEquals(cdapMessage.getOpCode(), Opcode.M_STOP_R);
			System.out.println("Application unregistered, stopping server");
			tcpServer.setEnd(true);
			Assert.assertTrue(rinaLibrarySocket.isClosed());
			
		}catch(Exception ex){
			ex.printStackTrace();
		}*/
	}
	
	private void wait2Seconds() throws Exception{
			Thread.sleep(2000);
	}
	
	private CDAPMessage getAllocateRequest() throws Exception{
		FlowService flowService = new FlowService();
		ApplicationProcessNamingInfo apNamingInfo = new ApplicationProcessNamingInfo("A", "1");
		flowService.setSourceAPNamingInfo(apNamingInfo);
		apNamingInfo = new ApplicationProcessNamingInfo("B", "1");
		flowService.setDestinationAPNamingInfo(apNamingInfo);
		byte[] encodedObject = encoder.encode(flowService);
		
		ObjectValue objectValue = new ObjectValue();
		objectValue.setByteval(encodedObject);
		CDAPMessage cdapMessage = 
			CDAPMessage.getCreateObjectRequestMessage(null, null, FlowService.OBJECT_CLASS, 0, FlowService.OBJECT_NAME, objectValue, 0);
		
		return cdapMessage;
	}
	
	private CDAPMessage getWriteDataRequest(byte[] sdu) throws Exception{
		ObjectValue objectValue = new ObjectValue();
		objectValue.setByteval(sdu);
		CDAPMessage cdapMessage = CDAPMessage.getWriteObjectRequestMessage(null, null, null, 0, objectValue, null, 0);
		return cdapMessage;
	}
	
	private CDAPMessage getDeallocateRequest() throws Exception{
		CDAPMessage cdapMessage = CDAPMessage.getDeleteObjectRequestMessage(null, null, null, 0, null, null, 0);
		return cdapMessage;
	}
	
	private CDAPMessage getRegistrationRequest(ApplicationRegistration applicationRegistration) throws Exception{
		byte[] encodedObject = encoder.encode(applicationRegistration);

		ObjectValue objectValue = new ObjectValue();
		objectValue.setByteval(encodedObject);
		CDAPMessage cdapMessage = CDAPMessage.
			getStartObjectRequestMessage(null, null, ApplicationRegistration.OBJECT_CLASS, objectValue, 0, ApplicationRegistration.OBJECT_NAME, 0);
		return cdapMessage;
	}
	
	/**
	 * Write a delimited CDAP message to the socket output stream
	 * @param socket
	 * @param cdapMessage
	 * @throws Exception
	 */
	private void sendCDAPMessage(Socket socket, CDAPMessage cdapMessage) throws Exception{
		byte[] encodedMessage = cdapSessionManager.encodeCDAPMessage(cdapMessage);
		byte[] delimitedMessage = delimiter.getDelimitedSdu(encodedMessage);
		socket.getOutputStream().write(delimitedMessage);
	}

}
