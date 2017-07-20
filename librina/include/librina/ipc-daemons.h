/*
 * Common librina classes for IPC Process and IPC Manager daemons
 *
 *    Eduard Grasa <eduard.grasa@i2cat.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301  USA
 */

#ifndef LIBRINA_IPC_DAEMONS_H
#define LIBRINA_IPC_DAEMONS_H

#ifdef __cplusplus

#include "common.h"

namespace rina {

/**
 * Thrown when there are problems assigning an IPC Process to a DIF
 */
class AssignToDIFException: public IPCException {
public:
        AssignToDIFException() :
                IPCException("Problems assigning IPC Process to DIF"){
        }
        AssignToDIFException(const std::string& description):
                IPCException(description){
        }
};

/**
 * Thrown when there are problems in the configuration
 */
class BadConfigurationException: public IPCException {
public:
	BadConfigurationException():
                IPCException("Problems assigning IPC Process to DIF"){
        }
	BadConfigurationException(const std::string& description):
                IPCException(description){
        }
};

/**
 * Thrown when there are problems updating a DIF configuration
 */
class UpdateDIFConfigurationException: public IPCException {
public:
        UpdateDIFConfigurationException():
                IPCException("Problems updating DIF configuration"){
        }
        UpdateDIFConfigurationException(const std::string& description):
                IPCException(description){
        }
};

/**
 * Thrown when there are problems instructing an IPC Process to enroll to a DIF
 */
class EnrollException: public IPCException {
public:
        EnrollException():
                IPCException("Problems causing an IPC Process to enroll to a DIF"){
        }
        EnrollException(const std::string& description):
                IPCException(description){
        }
};

/**
 * Thrown when there are problems while modifying a policy-set-related
 * parameter of an IPC Process
 */
class SetPolicySetParamException: public IPCException {
public:
	SetPolicySetParamException():
		IPCException("Problems while modifying a policy-set related "
                                "parameter of an IPC Process"){
	}
	SetPolicySetParamException(const std::string& description):
		IPCException(description){
	}
};

/**
 * Thrown when there are problems while selectin a policy-set
 * in an IPC Process component
 */
class SelectPolicySetException: public IPCException {
public:
	SelectPolicySetException():
		IPCException("Problems while selecting a policy-set "
                                "in an IPC Process component"){
	}
	SelectPolicySetException(const std::string& description):
		IPCException(description){
	}
};

/**
 * Thrown when there are problems while trying to load or unload
 * plugins
 */
class PluginLoadException: public IPCException {
public:
	PluginLoadException():
		IPCException("Problems while loading or unloading "
                                "a plugin for an IPC Process") {
	}
	PluginLoadException(const std::string& description):
		IPCException(description) {
	}
};

/**
 * Thrown when there are problems while trying to forward
 * a CDAP message.
 */
class FwdCDAPMsgException: public IPCException {
public:
	FwdCDAPMsgException():
		IPCException("Problems while forwarding a CDAP "
                                "message to an IPC Process") {
	}
	FwdCDAPMsgException(const std::string& description):
		IPCException(description) {
	}
};

/**
 * Event informing about the result of an assign to DIF operation
 */
class AssignToDIFResponseEvent: public BaseResponseEvent {
public:
        AssignToDIFResponseEvent(int result, unsigned int sequenceNumber,
        		unsigned int ctrl_p, unsigned short ipcp_id);
};

/**
 * An IPC process reports the result of the access of a policy-set-related
 * parameter
 */
class SetPolicySetParamResponseEvent: public IPCEvent {
public:
        int result;

	SetPolicySetParamResponseEvent(int result,
                                       unsigned int sequenceNumber,
				       unsigned int ctrl_p, unsigned short ipcp_id);
};

/**
 * An IPC process reports the result of the selection of a policy-set
 */
class SelectPolicySetResponseEvent: public IPCEvent {
public:
        int result;

	SelectPolicySetResponseEvent(int result,
                                     unsigned int sequenceNumber,
				     unsigned int ctrl_p, unsigned short ipcp_id);
};

/**
 * An IPC process reports the result of the loading or
 * unloading of a plugin
 */
class PluginLoadResponseEvent: public IPCEvent {
public:
        int result;

	PluginLoadResponseEvent(int result,
                                unsigned int sequenceNumber,
				unsigned int ctrl_p, unsigned short ipcp_id);
};

/**
 * The IPC Manager wants to forward a CDAP message to
 * an IPC process, or the IPC process wants to forwards a
 * CDAP request to the IPCManager
 */
class FwdCDAPMsgRequestEvent: public IPCEvent {
public:
	/** The serialized CDAP message to be forwarded */
	ser_obj_t sermsg;

	/** Result of the forwarding operation, used only
	 * in the direction IPC Process --> IPC Manager. */
	int result;

	FwdCDAPMsgRequestEvent(const ser_obj_t& sm, int result,
			unsigned int sequenceNumber,
			unsigned int ctrl_p, unsigned short ipcp_id);
};

/**
 * The IPC Manager wants to forward a CDAP response message to
 * an IPC process, or the IPC process wants to forwards a
 * CDAP response back to the IPC Manager.
 */
class FwdCDAPMsgResponseEvent: public IPCEvent {
public:
        /** The serialized CDAP message to be forwarded */
        ser_obj_t sermsg;

        /** Result of the forwarding operation, used only
         * in the direction IPC Process --> IPC Manager. */
        int result;

        FwdCDAPMsgResponseEvent(const ser_obj_t& sm, int result,
                        unsigned int sequenceNumber,
			unsigned int ctrl_p, unsigned short ipcp_id);
};

/**
 * Provides information on a base station IPCP
 */
class BaseStationInfo {
public:
        bool operator==(const BaseStationInfo &other) const;
        bool operator!=(const BaseStationInfo &other) const;
	static void from_c_bs_info(BaseStationInfo & bs,
				   struct bs_info_entry * bsi);
	struct bs_info_entry * to_c_bs_info(void) const;

	/* Address of the IPCP at the base station */
	std::string ipcp_address;

	/*
	 * The power level received from the IPCP, the higher
	 * the greater is the power level
	 */
	int signal_strength;

	std::string toString() const;
};

/**
 * Provides information on a DIF that manages a physical media,
 * resulting from a media scan
 */
class MediaDIFInfo {
public:
        bool operator==(const MediaDIFInfo &other) const;
        bool operator!=(const MediaDIFInfo &other) const;
	static void from_c_media_dif_info(MediaDIFInfo & md,
					  struct media_dif_info * mdi);
	struct media_dif_info * to_c_media_dif_info(void) const;

	/* Name of the DIF */
	std::string dif_name;

	/* Name of security policies */
	std::string security_policies;

	/* Information available of IPCPs in Base Stations / Access Points */
	std::list<BaseStationInfo> available_bs_ipcps;

	std::string toString() const;
};

/**
 * Captures the information resulting from a scan of the physical
 * media (WiFi, etc.)
 */
class MediaReport {
public:
        bool operator==(const MediaReport &other) const;
        bool operator!=(const MediaReport &other) const;
	static void from_c_media_report(MediaReport &mr,
					struct media_report * mre);
	struct media_report * to_c_media_report(void) const;

	/** The id of the IPC Process that scanned the media */
	unsigned short ipcp_id;

	/** The DIF the IPCP id is currently a member of (if any) */
	std::string current_dif_name;

	/**
	 * The address of the access point / base station the IPCP is
	 * currently attached to (if any)
	 */
	std::string bs_ipcp_address;

	/** Information on the DIFs available through the media */
	std::map<std::string, MediaDIFInfo> available_difs;

	std::string toString() const;
};

}

#endif

#endif
