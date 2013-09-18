package rina.protection.api;

import rina.efcp.api.PDU;
import rina.ipcservice.api.IPCException;

/**
 * An instantiation of an SDU protection module. Can protect 
 * a PDU by prepending some bytes, and unprotect the SDU by 
 * removing them. SDU Protection comprises integrity, confidentiality 
 * and compression. SDU Protection is also the responsible 
 * for encoding the bytes forming the PCI header, the user data and 
 * protection into a single SDU in an efficient way.
 * 
 * @author eduardgrasa
 *
 */
public interface SDUProtectionModule {
	
	/**
	 * Return the type of the SDU Protection Module
	 * @return
	 */
	public String getType();

	/**
	 * Protects a PDU before being sent through an N-1 flow
	 * @param pdu
	 * @return the protected PDU
	 * @throws IPCException if there is an issue protecting the PDU
	 */
	public byte[] protectPDU(PDU pdu) throws IPCException;
	
	/**
	 * Unprotects a PDU after receiving it from an N-1 flow. When this 
	 * call returns the PDU.rawPDU attribute must contain a byte array 
	 * that is the concatenation of the encoded PCI + the user data.
	 * @param sdu
	 * @return the unprotected PDU
	 * @throws IPCException if there is an issue unprotecting the SDU
	 */
	public PDU unprotectPDU(byte[] pdu) throws IPCException;
}
