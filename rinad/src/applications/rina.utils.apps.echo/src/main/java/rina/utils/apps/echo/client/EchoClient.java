package rina.utils.apps.echo.client;

import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

import eu.irati.librina.ApplicationProcessNamingInformation;
import eu.irati.librina.Flow;
import eu.irati.librina.FlowSpecification;
import eu.irati.librina.rina;

/**
 * Implements the behavior of a RINABand Client
 * @author eduardgrasa
 *
 */
public class EchoClient{
	
	private int numberOfSdus = 0;
	private int sduSize = 0;
	
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
	
	public EchoClient(int numberOfSdus, int sduSize, 
			ApplicationProcessNamingInformation echoApNamingInfo){
		this.numberOfSdus = numberOfSdus;
		this.sduSize = sduSize;
		this.echoApNamingInfo = echoApNamingInfo;
		this.clientApNamingInfo = new ApplicationProcessNamingInformation("rina.utils.apps.echoclient", "");
		this.executorService = Executors.newCachedThreadPool();
		rina.initialize();
	}
	
	public void execute(){
		try{
			//1 Allocate a flow to the Echo Server AE
			FlowSpecification qosSpec = new FlowSpecification();
			this.flow = rina.getIpcManager().allocateFlowRequest(
					this.clientApNamingInfo, this.echoApNamingInfo, qosSpec);
			
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
		}catch(Exception ex){
			ex.printStackTrace();
			System.exit(-1);
		}
	}	
}
