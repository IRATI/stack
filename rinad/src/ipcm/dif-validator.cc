#include "dif-validator.h"
#include <sstream>

#define RINA_PREFIX "ipcm.dif-validator"
#include <librina/logs.h>

namespace rinad {

DIFConfigValidator::DIFConfigValidator(const rina::DIFInformation &dif_info,
		std::string type)
		: dif_info_(dif_info)
{
	if (type == "normal-ipc")
		type_ = NORMAL;
	else if (type == "shim-dummy")
		type_ = SHIM_DUMMY;
	else if (type == "shim-eth-vlan")
		type_ = SHIM_ETH;
	else if (type == "shim-tcp-udp")
		type_ = SHIM_TCP_UDP;
	else if (type == "shim-hv")
		type_ = SHIM_HV;
	else
		type_ = SHIM_NOT_DEFINED;
}

bool DIFConfigValidator::validateConfigs()
{
	if (type_ == NORMAL)
		return validateNormal();
	else if (type_ == SHIM_ETH)
		return validateShimEth();
	else if (type_ == SHIM_DUMMY)
		return validateShimDummy();
	else if(type_ == SHIM_TCP_UDP)
		return validateShimTcpUdp();
	else if(type_ == SHIM_HV)
		return validateShimHv();
	else
		return validateBasicDIFConfigs();
}

bool DIFConfigValidator::validateShimEth()
{
	std::vector<std::string> expected_params;

	expected_params.push_back("interface-name");

	return validateBasicDIFConfigs() &&
		validateConfigParameters(expected_params);
}

bool DIFConfigValidator::validateShimTcpUdp()
{
	return validateBasicDIFConfigs();
}

bool DIFConfigValidator::validateShimHv()
{
	std::vector<std::string> expected_params;

	expected_params.push_back("vmpi-id");

	return validateBasicDIFConfigs() &&
		validateConfigParameters(expected_params);
}

bool DIFConfigValidator::validateShimDummy()
{
	return validateBasicDIFConfigs();
}


bool DIFConfigValidator::validateNormal()
{
	return  dataTransferConstants() && qosCubes() &&
			knownIPCProcessAddresses();
}

bool DIFConfigValidator::validateBasicDIFConfigs()
{
	return !dif_info_.dif_name_.processName.empty();
}

bool DIFConfigValidator::validateConfigParameters(
			const std::vector<std::string>& expected_params)
{
	std::vector<bool> found_params(expected_params.size());

	for (unsigned int i = 0; i < found_params.size(); i++) {
		found_params[i] = false;
	}

	for (unsigned int i = 0; i < expected_params.size(); i++) {
		for (std::list<rina::PolicyParameter>::const_iterator it =
			dif_info_.dif_configuration_.parameters_.begin();
				it != dif_info_.dif_configuration_.parameters_.end(); it++) {
			if (it->name_ == expected_params[i]) {
				found_params[i] = true;
				break;
			}
		}
	}

	for (unsigned int i = 0; i < found_params.size(); i++) {
		if (!found_params[i]) {
			return false;
		}
	}

	return true;
}

bool DIFConfigValidator::dataTransferConstants() {
	rina::DataTransferConstants data_trans_config = 
		dif_info_.dif_configuration_.efcp_configuration_
			.data_transfer_constants_;
	std::stringstream ss;
	ss << "Data Transfer Constants configuration failed: ";
	bool result = true;
	if(data_trans_config.address_length_ == 0)
	{
		result = false;
		ss<<"address_length, ";
	}
	if(data_trans_config.qos_id_length_ == 0)
	{
		result = false;
		ss<<"qos_id_length, ";
	}
	if(data_trans_config.port_id_length_ == 0)
	{
		result = false;
		ss<<"port_id_length, ";
	}
	if(data_trans_config.cep_id_length_ == 0)
	{
		result = false;
		ss<<"cep_id_length, ";
	}
	if(data_trans_config.sequence_number_length_ == 0)
	{
		result = false;
		ss<<"sequence_number_length, ";
	}
	if(data_trans_config.ctrl_sequence_number_length_ == 0)
	{
		result = false;
		ss<<"ctrl_sequence_number_length, ";
	}
	if(data_trans_config.length_length_ == 0)
	{
		result = false;
		ss<<"length_length, ";
	}
	if(data_trans_config.max_pdu_size_ == 0)
	{
		result = false;
		ss<<"max_pdu_size, ";
	}
	if(data_trans_config.rate_length_ == 0)
	{
		result = false;
		ss<<"rate_length, ";
	}
	if(data_trans_config.frame_length_ == 0)
	{
		result = false;
		ss<<"frame_length, ";
	}
	if(data_trans_config.max_pdu_lifetime_ == 0)
	{
		result = false;
		ss<<"max_pdu_lifetime, ";
	}
	if (!result)
	{
		std::stringstream result;
		print_log(ss.str(), result);
		LOG_ERR("%s", result.str().c_str());
	}

	return result;
}

bool DIFConfigValidator::qosCubes()
{
	std::string s = "";
	bool result =
		dif_info_.dif_configuration_.efcp_configuration_.qos_cubes_.begin()
		!= dif_info_.dif_configuration_.efcp_configuration_.qos_cubes_.end();
	if(!result)
		s  = s + "No QOS Cube defined";

	for (std::list<rina::QoSCube*>::const_iterator it = 
		dif_info_.dif_configuration_.efcp_configuration_.qos_cubes_.begin();
	     it != 
		 dif_info_.dif_configuration_.efcp_configuration_.qos_cubes_.end();
	     ++it) {
		bool temp_result =  !(*it)->name_.empty() &&
			(*it)->id_ != 0;
		if(!temp_result)
		{
			std::stringstream aux;
			aux << "QOS name or ID empty. Name: "<<(*it)->name_<< " ID: "<<(*it)->id_;
			s = s + aux.str();
		}
		result = result && temp_result;
	}

	if (!result)
	{
		std::stringstream result_msg;
		print_log(s, result_msg);
		LOG_ERR("%s", result_msg.str().c_str());
	}

	return result;
}

bool DIFConfigValidator::knownIPCProcessAddresses()
{
	std::list<rina::StaticIPCProcessAddress> staticAddress =
			dif_info_.dif_configuration_.nsm_configuration_.
			addressing_configuration_.static_address_;
	bool result = staticAddress.begin() != staticAddress.end();
	for (std::list<rina::StaticIPCProcessAddress>::iterator it =
			staticAddress.begin();
	     it != staticAddress.end();
	     ++it) {
		bool temp_result = !it->ap_name_.empty() && it->address_ != 0;
		result = result && temp_result;
	}

	if (!result)
	{
		std::stringstream result;
		print_log("Know IPCP Processes Addresses configuration failed", result);
		LOG_ERR("%s", result.str().c_str());
	}

	return result;
}

void DIFConfigValidator::print_log(const std::string& message, std::stringstream& result)
{
	result << "\n //////////////////////////////////////////////// \n" <<
			 "//////          DIF VALIDATOR ERROR    ///////// \n" <<
			 "//////" << message << "////////"<< "\n" <<
			 "////////////////////////////////////////////////";
}

} //rinad namespace
