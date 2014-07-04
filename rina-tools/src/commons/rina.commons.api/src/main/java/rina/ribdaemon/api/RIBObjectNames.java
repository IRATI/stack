package rina.ribdaemon.api;

/**
 * Contains the object names of the objects in the RIB
 * @author eduardgrasa
 */
public interface RIBObjectNames {
	/* Partial names */
	public static final String ADDRESS = "address";
	public static final String APNAME = "applicationprocessname";
	public static final String CONSTANTS = "constants";
	public static final String DATA_TRANSFER = "datatransfer";
	public static final String DAF = "daf";
	public static final String DIF = "dif";
	public static final String DIF_REGISTRATIONS = "difregistrations";
	public static final String DIRECTORY_FORWARDING_TABLE_ENTRIES = "directoryforwardingtableentries";
	public static final String ENROLLMENT = "enrollment";
	public static final String FLOWS = "flows";
	public static final String FLOW_ALLOCATOR = "flowallocator";
	public static final String IPC = "ipc";
	public static final String MANAGEMENT = "management";
	public static final String NEIGHBORS = "neighbors";
	public static final String NAMING = "naming";
	public static final String NMINUSONEFLOWMANAGER = "nminusoneflowmanager";
	public static final String NMINUSEONEFLOWS = "nminusoneflows";
	public static final String OPERATIONAL_STATUS = "operationalStatus";
	public static final String PDU_FORWARDING_TABLE = "pduforwardingtable";
	public static final String QOS_CUBES = "qoscubes";
	public static final String RESOURCE_ALLOCATION = "resourceallocation";
	public static final String ROOT = "root";
	public static final String SEPARATOR = "/";
	public static final String SYNONYMS = "synonyms";
	public static final String WHATEVERCAST_NAMES = "whatevercastnames";
	public static final String PDUFTG = "pduftgenerator";
	public static final String LINKSTATE = "pduftgenerator";
	public static final String FLOWSTATEOBJECTGROUP = "flowstateobjectgroup";
	
	/* Full names */
	public static final String OPERATIONAL_STATUS_RIB_OBJECT_NAME = RIBObjectNames.SEPARATOR + RIBObjectNames.DAF + 
		RIBObjectNames.SEPARATOR + RIBObjectNames.MANAGEMENT + RIBObjectNames.SEPARATOR + RIBObjectNames.OPERATIONAL_STATUS;
	
	public static final String OPERATIONAL_STATUS_RIB_OBJECT_CLASS = "operationstatus";
	
	public static final String PDU_FORWARDING_TABLE_RIB_OBJECT_CLASS = "pdu forwarding table";
	public static final String PDU_FORWARDING_TABLE_RIB_OBJECT_NAME = RIBObjectNames.SEPARATOR + RIBObjectNames.DIF + 
		RIBObjectNames.SEPARATOR + RIBObjectNames.RESOURCE_ALLOCATION + RIBObjectNames.SEPARATOR + RIBObjectNames.PDU_FORWARDING_TABLE;
}