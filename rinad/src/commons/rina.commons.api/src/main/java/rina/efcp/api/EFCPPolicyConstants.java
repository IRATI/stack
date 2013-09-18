package rina.efcp.api;

/**
 * Contains names for EFCP Policies and EFCP Policy Parameters
 * @author eduardgrasa
 *
 */
public interface EFCPPolicyConstants {
	public static final String CONSTANT = "constant";
	public static final String CREDIT = "credit";
	public static final String DISCARD = "discard";
	public static final String DTCP_FLOW_CONTROL = "dtcp.flowControl";
	public static final String DTCP_FLOW_CONTROL_FLOW_CONTROL_OVERRUN_POLICY = "dtcp.flowControl.flowControlOverrunPolicy";
	public static final String DTCP_FLOW_CONTROL_INITIAL_CREDIT_POLICY = "dtcp.flowControl.initialCreditPolicy";
	public static final String DTCP_FLOW_CONTROL_INITIAL_CREDIT_POLICY_CONSTANT_VALUE = "dtcp.flowControl.initialCreditPolicy.constantValue";
	public static final String DTCP_FLOW_CONTROL_RECEIVING_FLOW_CONTROL_POLICY = "dtcp.flowControl.receivingFlowControlPolicy";
	public static final String DTCP_FLOW_CONTROL_UPDATE_CREDIT_POLICY = "dtcp.flowControl.updateCreditPolicy";
	public static final String DTCP_FLOW_CONTROL_UPDATE_CREDIT_POLICY_CONSTANT_VALUE = "dtcp.flowControl.updateCreditPolicy.constantValue";
	public static final String EXHAUSTED = "exhausted";
}
