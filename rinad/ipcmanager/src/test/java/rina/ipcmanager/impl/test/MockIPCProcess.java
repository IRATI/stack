package rina.ipcmanager.impl.test;

import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

import junit.framework.Assert;

import rina.ipcmanager.impl.test.APNotifier.Status;
import rina.ipcprocess.api.BaseIPCProcess;
import rina.ipcservice.api.APService;
import rina.applicationprocess.api.ApplicationProcessNamingInfo;
import rina.ipcservice.api.FlowService;
import rina.ipcservice.api.IPCException;
import rina.ipcservice.api.IPCService;

public class MockIPCProcess extends BaseIPCProcess implements IPCService{
	
	private FlowService flowService = null;
	private APService apService = null;
	
	/**
	 * The thread pool implementation
	 */
	private ExecutorService executorService = null;
	
	public MockIPCProcess(){
		super(IPCProcessType.FAKE);
		executorService = Executors.newFixedThreadPool(2);
	}
	
	public void setAPService(APService apService){
		this.apService = apService;
	}
	
	public void setFlowService(FlowService flowService){
		this.flowService = flowService;
	}
	
	public void deliverDeallocateRequestToApplicationProcess(int arg0) {
		// TODO Auto-generated method stub
	}

	public void deliverSDUsToApplicationProcess(List<byte[]> arg0, int arg1) {
		// TODO Auto-generated method stub
	}

	public void destroy() {
		// TODO Auto-generated method stub
	}

	public void register(ApplicationProcessNamingInfo apNamingInfo, APService applicationCallback) {
		Assert.assertEquals(apNamingInfo.getApplicationProcessName(), "A");
		Assert.assertEquals(apNamingInfo.getApplicationProcessInstance(), "1");
		System.out.println("Registered application "+apNamingInfo.toString());
	}

	public int submitAllocateRequest(FlowService flowService, APService applicationCallback) throws IPCException {
		Assert.assertEquals(flowService.getDestinationAPNamingInfo().getApplicationProcessName(), "B");
		Assert.assertEquals(flowService.getDestinationAPNamingInfo().getApplicationProcessInstance(), "1");
		Assert.assertEquals(flowService.getSourceAPNamingInfo().getApplicationProcessName(), "A");
		Assert.assertEquals(flowService.getSourceAPNamingInfo().getApplicationProcessInstance(), "1");
		this.flowService = flowService;
		this.flowService.setPortId(1);
		
		System.out.println("Received allocate request from application process A-1 to communicate with application process B-1");
		System.out.println("Assigned portId: "+this.flowService.getPortId());
		APNotifier notifier = new APNotifier(apService, flowService, Status.ALLOCATE_RESPONSE_OK);
		executorService.execute(notifier);
		return this.flowService.getPortId();
	}

	public void submitAllocateResponse(int portId, boolean result, String reason, APService applicationCallback) throws IPCException {
		System.out.println("Allocate response called!!");
		Assert.assertEquals(portId, 24);
		Assert.assertEquals(result, true);
		Assert.assertNull(reason);
	}

	public void submitDeallocate(int portId) {
		System.out.println("Received deallocate request!!");
		System.out.println("Port id :" + portId);
		System.out.println(this.flowService.getPortId());
		Assert.assertEquals(portId, this.flowService.getPortId());
	}

	public void submitStatus(int arg0) {
		// TODO Auto-generated method stub
	}

	public void submitTransfer(int portId, byte[] sdu) throws IPCException {
		Assert.assertEquals(this.flowService.getPortId(), portId);
		System.out.println("Got a request for portId "+this.flowService.getPortId()+ " to send the following SDU: " + sdu);
		
		APNotifier notifier = new APNotifier(apService, flowService, Status.DELIVER_TRANSFER);
		executorService.execute(notifier);
	}

	public void unregister(ApplicationProcessNamingInfo apNamingInfo) {
		Assert.assertEquals(apNamingInfo.getApplicationProcessName(), "A");
		Assert.assertEquals(apNamingInfo.getApplicationProcessInstance(), "1");
		System.out.println("Unregistered application "+apNamingInfo.toString());
	}

	public Long getAddress() {
		// TODO Auto-generated method stub
		return null;
	}

	public void execute(Runnable arg0) {
		// TODO Auto-generated method stub
		
	}

	public String getApplicationProcessInstance() {
		// TODO Auto-generated method stub
		return null;
	}

	public String getApplicationProcessName() {
		// TODO Auto-generated method stub
		return null;
	}
}
