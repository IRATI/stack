package rina.configuration;

import java.util.ArrayList;
import java.util.List;

import rina.efcp.api.DataTransferConstants;
import rina.flowallocator.api.QoSCube;

/**
 * The configuration required to create a DIF
 * @author eduardgrasa
 *
 */
public class DIFConfiguration {

	/**
	 * The DIF Name
	 */
	private String difName = null;
	
	/**
	 * The DIF Data Transfer constants
	 */
	private DataTransferConstants dataTransferConstants = null;

	/**
	 * The QoS cubes available in the DIF
	 */
	private List<QoSCube> qosCubes = null;
	
	/**
	 * The possible policies of the DIF
	 */
	private List<Property> policies = null;
	
	/**
	 * The parameters associated to the policies of the DIF
	 */
	private List<Property> policyParameters = null;
	
	/**
	 * Only for normal DIFs
	 */
	private NMinusOneFlowsConfiguration nMinusOneFlowsConfiguration = null;
	
	/** Only for shim IP DIFs **/
	private List<ExpectedApplicationRegistration> expectedApplicationRegistrations = null;
	private List<DirectoryEntry> directory = null;
	
	/**
	 * The addresses of the known IPC Process (apname, address) 
	 * that can potentially be members of the DIFs I know
	 */
	private List<KnownIPCProcessAddress> knownIPCProcessAddresses = null;
	
	/**
	 * The address prefixes, assigned to different organizations
	 */
	private List<AddressPrefixConfiguration> addressPrefixes = null;

	public NMinusOneFlowsConfiguration getnMinusOneFlowsConfiguration() {
		return nMinusOneFlowsConfiguration;
	}
	public void setnMinusOneFlowsConfiguration(
			NMinusOneFlowsConfiguration nMinusOneFlowsConfiguration) {
		this.nMinusOneFlowsConfiguration = nMinusOneFlowsConfiguration;
	}
	
	public List<ExpectedApplicationRegistration> getExpectedApplicationRegistrations() {
		return expectedApplicationRegistrations;
	}

	public void setExpectedApplicationRegistrations(
			List<ExpectedApplicationRegistration> expectedApplicationRegistrations) {
		this.expectedApplicationRegistrations = expectedApplicationRegistrations;
	}
	
	public List<KnownIPCProcessAddress> getKnownIPCProcessAddresses() {
		return knownIPCProcessAddresses;
	}

	public void setKnownIPCProcessAddresses(
			List<KnownIPCProcessAddress> knownIPCProcessAddresses) {
		this.knownIPCProcessAddresses = knownIPCProcessAddresses;
	}

	public List<AddressPrefixConfiguration> getAddressPrefixes() {
		return addressPrefixes;
	}

	public void setAddressPrefixes(List<AddressPrefixConfiguration> addressPrefixes) {
		this.addressPrefixes = addressPrefixes;
	}

	public List<DirectoryEntry> getDirectory() {
		return directory;
	}

	public void setDirectory(List<DirectoryEntry> directory) {
		this.directory = directory;
	}
	
	public DataTransferConstants getDataTransferConstants() {
		return dataTransferConstants;
	}

	public void setDataTransferConstants(DataTransferConstants dataTransferConstants) {
		this.dataTransferConstants = dataTransferConstants;
	}

	public List<QoSCube> getQosCubes() {
		return qosCubes;
	}

	public void setQosCubes(List<QoSCube> qosCubes) {
		this.qosCubes = qosCubes;
	}
	
	public List<Property> getPolicies() {
		return policies;
	}
	public void setPolicies(List<Property> policies) {
		this.policies = policies;
	}
	public List<Property> getPolicyParameters() {
		return policyParameters;
	}
	public void setPolicyParameters(List<Property> policyParameters) {
		this.policyParameters = policyParameters;
	}
	public static DIFConfiguration getDefaultNormalDIFConfiguration(){
		DIFConfiguration result = new DIFConfiguration();
		result.setDifName("RINA-Demo.DIF");
		result.setDataTransferConstants(DataTransferConstants.getDefaultInstance());
		List<QoSCube> qosCubes = new ArrayList<QoSCube>();
		qosCubes.add(QoSCube.getDefaultReliableQoSCube());
		qosCubes.add(QoSCube.getDefaultUnreliableQoSCube());
		result.setQosCubes(qosCubes);
		
		return result;
	}

	public String getDifName() {
		return difName;
	}

	public void setDifName(String difName) {
		this.difName = difName;
	}
}
