package rina.utils.apps.rinaperf.server;

import java.util.Timer;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import rina.cdap.api.CDAPSessionManager;
import rina.cdap.api.message.CDAPMessage;
import rina.cdap.api.message.ObjectValue;
import rina.cdap.api.message.CDAPMessage.Opcode;
import rina.utils.apps.rinaperf.TestInformation;
import rina.utils.apps.rinaperf.protobuf.EchoTestMessageEncoder;

import eu.irati.librina.Flow;
import eu.irati.librina.rina;

/**
 * Reads sdus from a flow
 * @author eduardgrasa
 */
public class TestController implements Runnable {
	
	public static final long TIMER_PERIOD_IN_MS = 1000;
	
	private enum State {WAIT_START, EXECUTING, COMPLETED};
	
	/**
	 * The state of the test
	 */
	private State state = State.WAIT_START;
	
	private Flow flow;
	private int maxSDUSize;
	private boolean stop;
	private Timer timer = null;
	private CDAPSessionManager cdapSessionManager = null;
	private TestInformation testInformation = null;
	private int sdusReceived = 0;
	TestDeclaredDeadTimerTask deadTimerTask = null;
	
	private static final Log log = LogFactory.getLog(TestController.class);
	
	public TestController(Flow flow, int maxSDUSize, 
			CDAPSessionManager cdapSessionManager){
		this.flow = flow;
		this.maxSDUSize =  maxSDUSize;
		this.cdapSessionManager = cdapSessionManager;
		this.stop = false;
		this.timer = new Timer();
	}
	
	public synchronized void receivedSDU() {
		sdusReceived++;
	}
	
	public synchronized int getReceivedSDUs() {
		return sdusReceived;
	}

	@Override
	public void run() {
		byte[] buffer = new byte[maxSDUSize];
		int bytesRead = 0;
		
		deadTimerTask = new TestDeclaredDeadTimerTask(this);
		timer.schedule(deadTimerTask, 5000);
		
		while(!isStopped()){
			try{
				bytesRead = flow.readSDU(buffer, maxSDUSize);
				processSDU(buffer, bytesRead);
			}catch(Exception ex){
				log.error("Problems reading SDU from flow "+flow.getPortId());
				stop();
			}
		}
		
		if (flow.isAllocated()) {
			terminateReader();
		}
	}
	
	private byte[] getSDU(byte[] buffer, int bytesRead) {
		byte[] sdu = new byte[bytesRead];
		for(int i=0; i<bytesRead; i++) {
			sdu[i] = buffer[i];
		}
		
		return sdu;
	}
	
	private CDAPMessage getCDAPMessage(byte[] buffer,  int bytesRead) throws Exception {
		byte[] sdu = getSDU(buffer, bytesRead);
		return cdapSessionManager.decodeCDAPMessage(sdu);
	}
	
	private void sendCDAPMessage(CDAPMessage cdapMessage) throws Exception{
		byte[] sdu = cdapSessionManager.encodeCDAPMessage(cdapMessage);
		flow.writeSDU(sdu, sdu.length);
	}
	
	private void processSDU(byte[] buffer,  int bytesRead) {
		try {
			switch(this.state) {
			case WAIT_START:
				CDAPMessage cdapMessage = getCDAPMessage(buffer, bytesRead);
				if (cdapMessage.getOpCode().equals(Opcode.M_START)) {
					processStartTestMessage(cdapMessage);
				} else {
					log.error("Received CDAP message with wrong opcode while in " 
							+ state + " state: "+cdapMessage.getOpCode());
				}
				break;
			case EXECUTING:
				if (getReceivedSDUs() == 0) {
					deadTimerTask.cancel();
					StopTestTimerTask timerTask = new  StopTestTimerTask(this);
					timer.schedule(timerTask, testInformation.getTime()*1000);
				}
				receivedSDU();
				break;
			default:
				log.error("Undefined state");
			}
		} catch(Exception ex) {
			log.error("Error while processing SDU of "+bytesRead+" bytes while in "
					+state+" state. " + ex.getMessage());
		}
	}
	
	private void processStartTestMessage(CDAPMessage cdapMessage) throws Exception {
		ObjectValue objectValue = cdapMessage.getObjValue();
		if (objectValue == null || objectValue.getByteval() == null){
			log.error("The create message did not contain an object value. Ignoring the message");
			return;
		}
		
		this.testInformation = EchoTestMessageEncoder.decode(objectValue.getByteval());
		
		CDAPMessage replyMessage = cdapMessage.getReplyMessage();
		objectValue = new ObjectValue();
		objectValue.setByteval(EchoTestMessageEncoder.encode(testInformation));
		replyMessage.setObjValue(objectValue);
		sendCDAPMessage(replyMessage);
		this.state = State.EXECUTING;
		log.info("Ready for a test with the following parameters: \n"  
				+ this.testInformation.toString());
	}
	
	public synchronized void cancel() {
		if (!isStopped()){
			stop = true;
			terminateReader();
		}
	}
	
	public synchronized void stop(){
		if (!stop) {
			stop = true;
			log.info("Test terminated, received "+sdusReceived+" SDUs");
			if (testInformation != null) {
				testInformation.setSdusSent(getReceivedSDUs());
			}

			try{
				ObjectValue objectValue = new ObjectValue();
				objectValue.setByteval(EchoTestMessageEncoder.encode(testInformation));
				CDAPMessage replyMessage = CDAPMessage.getWriteObjectRequestMessage(null, null, "", 0, objectValue, "", 0);
				sendCDAPMessage(replyMessage);
			}catch(Exception ex) {
				log.error("Problems sending stats message to the client: "+ex.getMessage());
			}
		}
	}
	
	private void terminateReader() {
		if (flow.isAllocated()){
			try{
				rina.getIpcManager().requestFlowDeallocation(flow.getPortId());
			}catch(Exception ex){
				ex.printStackTrace();
			}
		}
		
		timer.cancel();
	}
	
	public synchronized boolean isStopped(){
		return stop;
	}
}
