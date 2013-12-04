package rina.utils.apps.echo;

import rina.utils.apps.echo.client.EchoClient;
import rina.utils.apps.echo.server.EchoServer;
import eu.irati.librina.ApplicationProcessNamingInformation;

public class Echo{
	
	public static final String DATA = "data";
	public static final String CONTROL = "control";
	public static final long MAX_TIME_WITH_NO_DATA_IN_MS = 2*1000;
	
	private EchoServer echoServer = null;
	private EchoClient echoClient = null;
	
	public Echo(boolean server, ApplicationProcessNamingInformation serverNamingInfo, 
			ApplicationProcessNamingInformation clientNamingInfo, int numberOfSDUs, int sduSize){
		if (server){
			echoServer = new EchoServer(serverNamingInfo);
		}else{
			echoClient = new EchoClient(numberOfSDUs, sduSize, serverNamingInfo, clientNamingInfo);
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
