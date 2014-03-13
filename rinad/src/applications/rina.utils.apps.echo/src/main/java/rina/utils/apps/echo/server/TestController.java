package rina.utils.apps.echo.server;

import java.util.Calendar;
import java.util.Timer;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import rina.cdap.api.CDAPSessionManager;
import rina.cdap.api.message.CDAPMessage;
import rina.cdap.api.message.ObjectValue;
import rina.cdap.api.message.CDAPMessage.Opcode;
import rina.utils.apps.echo.TestInformation;
import rina.utils.apps.echo.protobuf.EchoTestMessageEncoder;

import eu.irati.librina.Flow;
import eu.irati.librina.FlowDeallocationException;
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
	private long latestSDUReceivedTime = 0;
	private CDAPSessionManager cdapSessionManager = null;
	private TestInformation testInformation = null;
	
	private static final Log log = LogFactory.getLog(TestController.class);
	
	public TestController(Flow flow, int maxSDUSize, 
			CDAPSessionManager cdapSessionManager){
		this.flow = flow;
		this.maxSDUSize =  maxSDUSize;
		this.cdapSessionManager = cdapSessionManager;
		this.stop = false;
		this.timer = new Timer();
	}
	
	private synchronized void setLatestSDUReceivedTime(long time){
		this.latestSDUReceivedTime = time;
	}
	
	private synchronized long getLatestSDUReceivedTime(){
		return latestSDUReceivedTime;
	}

	@Override
	public void run() {
		byte[] buffer = new byte[maxSDUSize];
		int bytesRead = 0;
		
		TestDeclaredDeadTimerTask timerTask = new TestDeclaredDeadTimerTask(this, timer);
		timer.schedule(timerTask, TIMER_PERIOD_IN_MS);
		setLatestSDUReceivedTime(Calendar.getInstance().getTimeInMillis());
		
		while(!isStopped()){
			try{
				bytesRead = flow.readSDU(buffer, maxSDUSize);
				setLatestSDUReceivedTime(Calendar.getInstance().getTimeInMillis());
				processSDU(buffer, bytesRead);
			}catch(Exception ex){
				log.error("Problems reading SDU from flow "+flow.getPortId());
				stop();
			}
		}
		
		terminateReader();
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
				flow.writeSDU(buffer, bytesRead);
				testInformation.sduReceived();
				if (testInformation.receivedAllSDUs()) {
					state = State.COMPLETED;
					log.info("Received all SDUs, stopping the test");
					
					try{
						Thread.sleep(5);
					} catch(Exception ex){
					}
					
					stop();
				}
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
	
	public boolean shouldStop(){
		if (testInformation == null) {
			return false;
		}

		if (getLatestSDUReceivedTime() + testInformation.getTimeout() < 
				Calendar.getInstance().getTimeInMillis()) {
			return true;
		}
		
		return false;
	}
	
	public synchronized void stop(){
		if (!stop) {
			log.info("Requesting reader of flow "+flow.getPortId()+ " to stop");
			stop = true;
			
			if (testInformation != null && !testInformation.receivedAllSDUs()) {
				log.info("Stopping since more than "+ testInformation.getTimeout() 
						+ " ms have gone by without receiving SDUs");
				log.info("Received "+testInformation.getSDUsReceived() 
						+ " out of " + testInformation.getNumberOfSDUs() + " SDUs");
			}
		}
		
		terminateReader();
	}
	
	private void terminateReader() {
		if (flow.isAllocated()){
			try{
				rina.getIpcManager().requestFlowDeallocation(flow.getPortId());
			}catch(FlowDeallocationException ex){
				ex.printStackTrace();
			}
		}
		
		timer.cancel();
	}
	
	public synchronized boolean isStopped(){
		return stop;
	}
}
