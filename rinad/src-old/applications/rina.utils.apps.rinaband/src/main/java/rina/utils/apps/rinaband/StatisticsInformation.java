package rina.utils.apps.rinaband;

/**
 * Contains the statistics exchanged between the 
 * RINABand Client and the RINABand Server after 
 * each test
 * @author eduardgrasa
 *
 */
public class StatisticsInformation {
	
	/**
	 * Time when the client sent the first SDU, in units of microseconds since the UNIX Epoch
	 */
	private long clientTimeFirstSDUSent = 0L;
	
	/**
	 * Time when the client sent the last SDU, in units of microseconds since the UNIX Epoch
	 */
	private long clientTimeLastSDUSent = 0L;

	/**
	 * Time when the client received the first SDU, in units of microseconds since the UNIX Epoch
	 */
	private long clientTimeFirstSDUReceived = 0L;
	
	/**
	 * Time when the client received the last SDU, in units of microseconds since the UNIX Epoch
	 */
	private long clientTimeLastSDUReceived = 0L;
	
	/**
	 * Time when the server sent the first SDU, in units of microseconds since the UNIX Epoch
	 */
	private long serverTimeFirstSDUSent = 0L;
	
	/**
	 * Time when the server sent the last SDU, in units of microseconds since the UNIX Epoch
	 */
	private long serverTimeLastSDUSent = 0L;
	
	/**
	 * Time when the server received the first SDU, in units of microseconds since the UNIX Epoch
	 */
	private long serverTimeFirstSDUReceived = 0L;
	
	/**
	 * Time when the server received the last SDU, in units of microseconds since the UNIX Epoch
	 */
	private long serverTimeLastSDUReceived = 0L;

	public long getClientTimeFirstSDUReceived() {
		return clientTimeFirstSDUReceived;
	}

	public void setClientTimeFirstSDUReceived(long clientTimeFirstSDUReceived) {
		this.clientTimeFirstSDUReceived = clientTimeFirstSDUReceived;
	}

	public long getClientTimeLastSDUReceived() {
		return clientTimeLastSDUReceived;
	}

	public void setClientTimeLastSDUReceived(long clientTimeLastSDUReceived) {
		this.clientTimeLastSDUReceived = clientTimeLastSDUReceived;
	}

	public long getServerTimeFirstSDUReceived() {
		return serverTimeFirstSDUReceived;
	}

	public void setServerTimeFirstSDUReceived(long serverTimeFirstSDUReceived) {
		this.serverTimeFirstSDUReceived = serverTimeFirstSDUReceived;
	}

	public long getServerTimeLastSDUReceived() {
		return serverTimeLastSDUReceived;
	}

	public void setServerTimeLastSDUReceived(long serverTimeLastSDUReceived) {
		this.serverTimeLastSDUReceived = serverTimeLastSDUReceived;
	}

	public long getClientTimeFirstSDUSent() {
		return clientTimeFirstSDUSent;
	}

	public void setClientTimeFirstSDUSent(long clientTimeFirstSDUSent) {
		this.clientTimeFirstSDUSent = clientTimeFirstSDUSent;
	}

	public long getClientTimeLastSDUSent() {
		return clientTimeLastSDUSent;
	}

	public void setClientTimeLastSDUSent(long clientTimeLastSDUSent) {
		this.clientTimeLastSDUSent = clientTimeLastSDUSent;
	}

	public long getServerTimeFirstSDUSent() {
		return serverTimeFirstSDUSent;
	}

	public void setServerTimeFirstSDUSent(long serverTimeFirstSDUSent) {
		this.serverTimeFirstSDUSent = serverTimeFirstSDUSent;
	}

	public long getServerTimeLastSDUSent() {
		return serverTimeLastSDUSent;
	}

	public void setServerTimeLastSDUSent(long serverTimeLastSDUSent) {
		this.serverTimeLastSDUSent = serverTimeLastSDUSent;
	}

	public String toString(){
		String result = "";
		result = result + "Client first SDU sent time: " + this.clientTimeFirstSDUSent + "\n";
		result = result + "Client first SDU received time: "+this.clientTimeFirstSDUReceived + "\n";
		result = result + "Client last SDU sent time: " + this.clientTimeLastSDUSent + "\n";
		result = result + "Client last SDU received time: " + this.clientTimeLastSDUReceived + "\n";
		result = result + "Server first SDU sent time: " + this.serverTimeFirstSDUSent + "\n";
		result = result + "Server first SDU received time: " + this.serverTimeFirstSDUReceived + "\n";
		result = result + "Server last SDU sent time: " + this.serverTimeLastSDUSent + "\n";
		result = result + "Server last SDU received time: " + this.serverTimeLastSDUReceived + "\n";
		
		return result;
	}
}
