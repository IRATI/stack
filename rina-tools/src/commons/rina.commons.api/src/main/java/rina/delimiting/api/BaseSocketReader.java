package rina.delimiting.api;

import java.io.BufferedInputStream;
import java.io.IOException;
import java.net.Socket;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

/**
 * This class is a base socket reader that is continuously reading a socket looking for 
 * delimited pdus. When it finds one it will call the abstract operation processPDU. If the socket 
 * ends or is disconnected, then the abstract operation socketClosed is invoked.
 * @author eduardgrasa
 *
 */
public abstract class BaseSocketReader implements Runnable{

	private static final Log log = LogFactory.getLog(BaseSocketReader.class);
	private static final int MAX_SDU_LENGTH_SIZE = 5;
	
	private Delimiter delimiter = null;
	private Socket socket = null;
	
	/**
	 * Controls when the reader will finish the execution
	 */
	private boolean end = false;
	
	public BaseSocketReader(Socket socket, Delimiter delimiter){
		this.socket = socket;
		this.delimiter = delimiter;
	}
	
	public Delimiter getDelimiter(){
		return delimiter;
	}

	public void run() {
		boolean lookingForSduLength = true;
		int length = 0;
		byte[] sduLengthCandidate = new byte[MAX_SDU_LENGTH_SIZE];
		int sduLengthPosition = 0;
		byte nextByte = 0;
		int value = 0;
		
		byte[] pdu = null;
		int bytesRead = 0;
		int bytesToRead = 0;
		int offset = 0;
		
		BufferedInputStream bufInStream = null;
		//byte[] nextBytes = new byte[MAX_SDU_SIZE];
		//int numberOfBytesRead = 0;
		
		log.debug("Reading socket from remote interface: "+socket.getInetAddress().getHostAddress() + "\n" 
				+ "Local port_id: "+socket.getLocalPort() + "\n" 
				+ "Remote port_id: "+socket.getPort());
		
		try{
			bufInStream = new BufferedInputStream(socket.getInputStream());
		}catch(Exception ex){
			log.error("Socket reader stopping. "+ex.getMessage());
			return;
		}
		
		while(!end){
			//Delimit the byte array that contains a serialized CDAP message
			try{
				if (lookingForSduLength){
					value = bufInStream.read();
					if (value == -1){
						break;
					}

					nextByte = (byte) value;
					sduLengthCandidate[sduLengthPosition] = nextByte;
					length = delimiter.readVarint32(sduLengthCandidate, sduLengthPosition + 1);
					if (length == -2){
						sduLengthPosition++;
					}else if (length == -1){
						log.error("Detected SDU with SDU length encoded in too many bytes");
						sduLengthPosition = 0;
					}else{
						sduLengthPosition = 0;
						if (length > 0){
							lookingForSduLength = false;
						}
					}
				}else{
					if (pdu == null){
						pdu = new byte[length];
						bytesToRead = length;
						offset = 0;
					}
					
					bytesRead = bufInStream.read(pdu, offset, bytesToRead);
					if (bytesRead == -1){
						break;
					}
					
					offset = offset + bytesRead;
					bytesToRead = bytesToRead - bytesRead;
					if (bytesToRead == 0){
						try{
							processPDU(pdu);
						}catch(Exception ex){
							log.error("Problems when processing PDU: "+ex.getMessage()+ "\n.PDU: "+printBytes(pdu));
						}
						
						lookingForSduLength = true;
						pdu = null;
					}
				}
			}catch(Exception ex){
				end = true;
			}
		}
		
		try{
			log.debug("EOF detected, closing the socket");
			socket.close();
			log.debug("Socket closed");
		}catch(IOException ex){
			log.error("Exception closing the socket: "+ex.getMessage());
		}
		
		log.info("The remote endpoint of socket "+socket.getPort()+" has disconnected");
		socketDisconnected();
	}
	
	public Socket getSocket(){
		return this.socket;
	}
	
	/**
	 * process the pdu that has been found
	 * @param pdu
	 */
	public abstract void processPDU(byte[] pdu);
	
	/**
	 * Invoked when the socket is disconnected
	 */
	public abstract void socketDisconnected();
	
	public String printBytes(byte[] message){
		String result = "";
		for(int i=0; i<message.length; i++){
			result = result + String.format("%02X", message[i]) + " ";
		}
		
		return result;
	}
}
