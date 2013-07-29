package rina.ipcservice.api;

/**
 * Root class of exceptions
 * @author eduardgrasa
 *
 */
public class IPCException extends Exception{

	private static final long serialVersionUID = -4765335465449082765L;
	
	/** Error codes **/
	public static final int APPLICATION_NOT_SPECIFIED_CODE = 1;
	public static final int SOURCE_APPLICATION_NOT_SPECIFIED_CODE = 2;
	public static final int DESTINATION_APPLICATION_NOT_SPECIFIED_CODE = 3;
	public static final int SDU_LISTENER_NOT_SPECIFIED_CODE = 4;
	public static final int APPLICATION_PROCESS_NAME_NOT_SPECIFIED_CODE = 5;
	public static final int ERROR_ALLOCATING_THE_FLOW_CODE = 6;
	public static final int APPLICATION_UNREGISTERED_CODE = 7;
	public static final int NON_BLOCKING_REGISTRATION_CODE = 8;
	public static final int PROBLEMS_REGISTERING_APPLICATION_CODE = 9;
	public static final int PROBLEMS_ACCEPTING_FLOW_CODE = 10;
	public static final int PROBLEMS_UNREGISTERING_APPLICATION_CODE = 11;
	public static final int SOCKET_IS_ALREADY_PRESENT_CODE = 12;
	public static final int PROVIDED_SOCKET_NULL_CODE = 13;
	public static final int PROVIDED_SOCKET_NOT_CONNECTED_CODE = 14;
	public static final int PROBLEMS_WRITING_TO_FLOW_CODE = 15;
	public static final int FLOW_NOT_IN_ALLOCATED_STATE_CODE = 16;
	public static final int FLOW_IS_ALREADY_ALLOCATED_CODE = 17;
	public static final int PROBLEMS_ALLOCATING_FLOW_CODE = 18;
	public static final int PROBLEMS_DEALLOCATING_FLOW_CODE = 19;
	public static final int MALFORMED_ALLOCATE_REQUEST_CODE = 20;
	public static final int PORTID_NOT_IN_ALLOCATION_PENDING_STATE_CODE = 21;
	public static final int PORTID_NOT_IN_TRANSFER_STATE_CODE = 22;
	public static final int PORTID_NOT_IN_DEALLOCATION_PENDING_STATE_CODE = 23;
	public static final int NO_FLOW_ALLOCATOR_INSTANCE_FOR_THIS_PORTID_CODE = 24;
	public static final int COULD_NOT_FIND_ENTRY_IN_DIRECTORY_FORWARDING_TABLE_CODE = 25;
	public static final int PROBLEMS_SENDING_SDU_CODE = 26;
	public static final int NO_AVAILABLE_ADDRESSES_CODE = 27;
	public static final int ENROLLMENT_PROBLEM_CODE = 28;
	public static final int UNKNOWN_IPC_PROCESS_CODE = 29;
	public static final int APPLICATION_ALREADY_REGISTERED_CODE = 30;
	public static final int APPLICATION_ALREADY_UNREGISTERED_CODE = 31;
	public static final int PROBLEMS_RESERVING_CEP_IDS_CODE = 32;
	public static final int ERROR_CODE = 33;
	
	public static final String APPLICATION_NOT_SPECIFIED = "Application name not specified";
	public static final String SOURCE_APPLICATION_NOT_SPECIFIED = "Source application name not specified";
	public static final String DESTINATION_APPLICATION_NOT_SPECIFIED = "Destination application name not specified";
	public static final String SDU_LISTENER_NOT_SPECIFIED = "SDU Listener not specified";
	public static final String APPLICATION_PROCESS_NAME_NOT_SPECIFIED = "Application process name not specified";
	public static final String ERROR_ALLOCATING_THE_FLOW_= "Error allocating the flow";
	public static final String APPLICATION_UNREGISTERED = "The application is already unregistered";
	public static final String NON_BLOCKING_REGISTRATION = "This application registration object is configured in non-blocking mode, " +
			"therefore calls to the 'accept' operation are fobidden";
	public static final String PROBLEMS_REGISTERING_APPLICATION = "Problems registering application. ";
	public static final String PROBLEMS_ACCEPTING_FLOW = "Problems accepting flow. ";
	public static final String PROBLEMS_UNREGISTERING_APPLICATION = "Problems unregistering application. ";
	public static final String SOCKET_IS_ALREADY_PRESENT = "The socket is already present in this flow implementation object";
	public static final String PROVIDED_SOCKET_NULL= "The provided socket is null";
	public static final String PROVIDED_SOCKET_NOT_CONNECTED = "The provided socket is not connected";
	public static final String PROBLEMS_WRITING_TO_FLOW = "Problems sending data through the flow. ";
	public static final String FLOW_NOT_IN_ALLOCATED_STATE = "The flow is not in the allocated state.";
	public static final String FLOW_IS_ALREADY_ALLOCATED = "The flow is already allocated";
	public static final String PROBLEMS_ALLOCATING_FLOW = "Problems allocating the flow. ";
	public static final String PROBLEMS_DEALLOCATING_FLOW = "Problems deallocating the flow. ";
	public static final String MALFORMED_ALLOCATE_REQUEST = "Malformed allocate request";
	public static final String PORTID_NOT_IN_ALLOCATION_PENDING_STATE = "This portId is not in allocation pending state";
	public static final String PORTID_NOT_IN_TRANSFER_STATE = "This portId is not in transfer state";
	public static final String PORTID_NOT_IN_DEALLOCATION_PENDING_STATE = "This portId is not in deallocation pending state";
	public static final String NO_FLOW_ALLOCATOR_INSTANCE_FOR_THIS_PORTID = "There is no flow allocator instance associated to this portId";
	public static final String COULD_NOT_FIND_ENTRY_IN_DIRECTORY_FORWARDING_TABLE = "Could not find an entry in the directory forwarding table. ";
	public static final String PROBLEMS_SENDING_SDU = "Problems sending SDU through flow. ";
	public static final String NO_AVAILABLE_ADDRESSES = "There are no more available addresses in this DIF";
	public static final String ENROLLMENT_PROBLEM = "Problems during enrollment. ";
	public static final String UNKNOWN_IPC_PROCESS = "Unknown IPC Process. ";
	public static final String APPLICATION_ALREADY_REGISTERED = "The application is already registered";
	public static final String APPLICATION_ALREADY_UNREGISTERED = "The application is already unregistered";
	public static final String PROBLEMS_RESERVING_CEP_IDS = "Problems reserving Connection Endpoint IDs: either no more available or the flow had already CEP ids allocated";
	
	private int errorCode = 0;
	
	public IPCException(String message){
		super(message);
	}
	
	public IPCException(int errorCode){
		super();
		this.errorCode = errorCode;
	}
	
	public IPCException(int errorCode, String message){
		super(message);
		this.errorCode = errorCode;
	}

	public int getErrorCode() {
		return errorCode;
	}

	public void setErrorCode(int errorCode) {
		this.errorCode = errorCode;
	}
}
