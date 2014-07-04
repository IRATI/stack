package rina.utils.apps.rinaperf;

import rina.utils.apps.rinaperf.client.RINAPerfClient;
import rina.utils.apps.rinaperf.server.RINAPerfServer;
import eu.irati.librina.ApplicationProcessNamingInformation;

public class RINAPerf{
	
	public static final String DATA = "data";
	public static final String CONTROL = "control";
	
	private RINAPerfServer RINAPerfServer = null;
	private RINAPerfClient RINAPerfClient = null;
	
	public RINAPerf(boolean server, ApplicationProcessNamingInformation serverNamingInfo, 
			ApplicationProcessNamingInformation clientNamingInfo, int sduSize, 
			int time, int rate, int gap){
		if (server){
			RINAPerfServer = new RINAPerfServer(serverNamingInfo);
		}else{
			RINAPerfClient = new RINAPerfClient(sduSize, serverNamingInfo, clientNamingInfo, time, rate, gap);
		}
	}
	
	public void execute(){
		if (RINAPerfServer != null){
			RINAPerfServer.execute();
		}else{
			RINAPerfClient.execute();
		}
	}
}
