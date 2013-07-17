package rina.ipcprocess.api;

import java.util.List;

import rina.cdap.api.CDAPSessionManagerFactory;
import rina.configuration.RINAConfiguration;
import rina.delimiting.api.DelimiterFactory;
import rina.encoding.api.EncoderFactory;
import rina.ipcmanager.api.IPCManager;

/**
 * Creates, stores and destroys instances of IPC processes
 * @author eduardgrasa
 *
 */
public interface IPCProcessFactory {
	
	public static final String NORMAL = "normal";
	public static final String SHIMIP = "shimip";
	
	/**
	 * Creates a new IPC process
	 * @param applicationProcessName the application process name of this IPC process
	 * @param applicationProcessInstance the application process instance of this IPC process
	 */
	public IPCProcess createIPCProcess(String applicationProcessName, String applicationProcessInstance, RINAConfiguration config) throws Exception;
	
	/**
	 * Destroys an existing IPC process
	 * @param applicationProcessName the application process name of this IPC process
	 * @param applicationProcessInstance the application process instance of this IPC process
	 */
	public void destroyIPCProcess(String applicationProcessName, String applicationProcessInstance) throws Exception;
	
	/**
	 * Destroys an existing IPC process
	 * TODO add more stuff probably
	 * @param IPCService ipcProcess
	 */
	public void destroyIPCProcess(IPCProcess ipcProcess) throws Exception;
	
	/**
	 * Get an existing IPC process
	 * @param applicationProcessName the application process name of this IPC process
	 * @param applicationProcessInstance the application process instance of this IPC process
	 */
	public IPCProcess getIPCProcess(String applicationProcessName, String applicationProcessInstance);
	
	/**
	 * Return the IPC process that is a member of the DIF called "difname"
	 * @param difname The name of the DIF
	 * @return
	 */
	public IPCProcess getIPCProcessBelongingToDIF(String difname);
	
	/**
	 * Return a list of the existing IPC processes
	 * @return
	 */
	public List<IPCProcess> listIPCProcesses();
	
	/**
	 * Return a list of the names of the DIFs currently available in the system
	 * @return
	 */
	public List<String> listDIFNames();
	
	/**
	 * Set the IPCManager of this system
	 * @param ipcManager
	 */
	public void setIPCManager(IPCManager ipcManager);
	
	public CDAPSessionManagerFactory getCDAPSessionManagerFactory();
	public EncoderFactory getEncoderFactory();
	public DelimiterFactory getDelimiterFactory();
}
