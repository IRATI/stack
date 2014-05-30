package rina.configuration;

import java.util.List;

import eu.irati.librina.DataTransferConstants;
import eu.irati.librina.PDUFTableGeneratorConfiguration;
import eu.irati.librina.Parameter;
import eu.irati.librina.QoSCube;
import eu.irati.librina.RMTConfiguration;

/**
 * The configuration required to create a DIF
 * @author eduardgrasa
 *
 */
public class DIFProperties {

	/**
	 * The DIF Name
	 */
	private String difName = null;
	
	/**
	 * The DIF Type
	 */
	private String difType = null;
	
	/**
	 * The DIF Data Transfer constants
	 */
	private DataTransferConstants dataTransferConstants = null;

	/**
	 * The QoS cubes available in the DIF
	 */
	private List<QoSCube> qosCubes = null;
	
	private RMTConfiguration rmtConfiguration = null;
	
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
	 * The PDU forwarding table configurations
	 */
	private PDUFTableGeneratorConfiguration pdufTableGeneratorConfiguration = null;
	
	/**
	 * The address prefixes, assigned to different organizations
	 */
	private List<AddressPrefixConfiguration> addressPrefixes = null;
	
	/**
	 * Extra configuration parameters (name/value pairs)
	 */
	private List<Parameter> configParameters = null;

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
	public RMTConfiguration getRmtConfiguration() {
		return rmtConfiguration;
	}
	public void setRmtConfiguration(RMTConfiguration rmtConfiguration) {
		this.rmtConfiguration = rmtConfiguration;
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

	public String getDifName() {
		return difName;
	}

	public void setDifName(String difName) {
		this.difName = difName;
	}
	public String getDifType() {
		return difType;
	}
	public void setDifType(String difType) {
		this.difType = difType;
	}
	
	public List<Parameter> getConfigParameters() {
		return configParameters;
	}

	public void setConfigParameters(List<Parameter> configParameters) {
		this.configParameters = configParameters;
	}
	public PDUFTableGeneratorConfiguration getPdufTableGeneratorConfiguration() {
		return pdufTableGeneratorConfiguration;
	}
	public void setPdufTableGeneratorConfiguration(
			PDUFTableGeneratorConfiguration pdufTableGeneratorConfiguration) {
		this.pdufTableGeneratorConfiguration = pdufTableGeneratorConfiguration;
	}
}
