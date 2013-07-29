package rina.ipcmanager.impl.test;

import java.io.IOException;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

import rina.cdap.api.CDAPSessionManager;
import rina.delimiting.api.Delimiter;
import rina.encoding.api.Encoder;

/**
 * Server that listens for incoming 
 * @author eduardgrasa
 *
 */
public class TCPServer implements Runnable {
	
	/**
	 * Controls when the server will finish the execution
	 */
	private boolean end = false;
	
	/**
	 * The server socket that listens for incoming connections
	 */
	private ServerSocket serverSocket = null;
	
	private Delimiter delimiter = null;
	
	private CDAPSessionManager cdapSessionManager = null;
	
	private Encoder encoder = null;
	
	private ExecutorService executorService = null;
	
	public TCPServer(Delimiter delimiter, CDAPSessionManager cdapSessionManager, Encoder encoder){
		this.delimiter = delimiter;
		this.cdapSessionManager = cdapSessionManager;
		this.encoder = encoder;
		executorService = Executors.newFixedThreadPool(10);
		try {
			serverSocket = new ServerSocket(0);
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}
	
	public int getPort(){
		return serverSocket.getLocalPort();
	}
	
	public void setEnd(boolean end){
		this.end = end;
		if (end){
			try{
				this.serverSocket.close();
			}catch(IOException ex){
				ex.printStackTrace();
			}
		}
	}

	public void run() {
		try{
			System.out.println("Application waiting for incoming TCP connections from other applications on port "+getPort());
			while (!end){
				Socket socket = serverSocket.accept();
				String address = socket.getInetAddress().getHostAddress();
				String hostname = socket.getInetAddress().getHostName();
				
				//Accept only local connections
				if (!address.equals("127.0.0.1") && !hostname.equals("localhost")){
					System.out.println("Connection attempt from "+address+" blocked");
					socket.close();
					continue;
				}
				
				System.out.println("Got a new request from "+socket.getInetAddress().getHostAddress() + 
						". Local port: "+socket.getLocalPort()+"; Remote port: "+socket.getPort());
				
				//TODO
				SocketReader socketReader = new SocketReader(socket, delimiter, cdapSessionManager, null);
				socketReader.setEncoder(encoder);
				executorService.execute(socketReader);
			}
		}catch(IOException e){
			e.printStackTrace();
		}
		
	}

}
