package rina.utils.apps.echo;

import rina.utils.apps.echo.client.EchoClient;
import rina.utils.apps.echo.server.EchoServer;
import eu.irati.librina.ApplicationProcessNamingInformation;

public class Echo{
	
	public static final String DATA = "data";
	public static final String CONTROL = "control";
	
	private EchoServer echoServer = null;
	private EchoClient echoClient = null;
	
	public Echo(boolean server, String apName, String apInstance, int numberOfSDUs, int sduSize){
		ApplicationProcessNamingInformation echoApNamingInfo = 
				new ApplicationProcessNamingInformation(apName, apInstance);
		if (server){
			echoServer = new EchoServer(echoApNamingInfo);
		}else{
			echoClient = new EchoClient(numberOfSDUs, sduSize, echoApNamingInfo);
		}
	}
	
	public void execute(){
		if (echoServer != null){
			echoServer.execute();
		}else{
			echoClient.execute();
		}
	}
}
