#include "dif-validator.h"

#define RINA_PREFIX "ipcm.dif-validator"
#include <librina/logs.h>

using namespace std;

namespace rinad {

DIFConfigValidator::DIFConfigValidator(const rina::DIFConfiguration &dif_config,
		const rina::DIFInformation &dif_info, std::string type)
		:dif_config_(dif_config), dif_info_(dif_info)
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
	vector<string> expected_params;

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
	vector<string> expected_params;

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
			const vector<string>& expected_params)
{
	vector<bool> found_params(expected_params.size());

	for (unsigned int i = 0; i < found_params.size(); i++) {
		found_params[i] = false;
	}

	for (unsigned int i = 0; i < expected_params.size(); i++) {
		for (std::list<rina::Parameter>::const_iterator it =
			dif_config_.parameters_.begin();
				it != dif_config_.parameters_.end(); it++) {
			if (it->name == expected_params[i]) {
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
	rina::DataTransferConstants data_trans_config = dif_config_.efcp_configuration_
			.data_transfer_constants_;
	bool result = data_trans_config.address_length_ != 0 &&
		      data_trans_config.qos_id_length_ != 0 &&
		      data_trans_config.port_id_length_ != 0 &&
		      data_trans_config.cep_id_length_ != 0 &&
		      data_trans_config.sequence_number_length_ != 0 &&
		      data_trans_config.length_length_ != 0 &&
		      data_trans_config.max_pdu_size_ != 0 &&
		      data_trans_config.max_pdu_lifetime_ != 0;
	if (!result)
		LOG_ERR("Data Transfer Constants configuration failed");
	return result;
}

bool DIFConfigValidator::qosCubes()
{
	bool result =
		dif_config_.efcp_configuration_.qos_cubes_.begin()
		!= dif_config_.efcp_configuration_.qos_cubes_.end();

	for (std::list<rina::QoSCube*>::const_iterator it = dif_config_.
		     efcp_configuration_.qos_cubes_.begin();
	     it != dif_config_.efcp_configuration_.qos_cubes_.end();
	     ++it) {
		bool temp_result =  !(*it)->name_.empty() &&
			(*it)->id_ != 0;
		result = result && temp_result;
	}

	if (!result)
		LOG_ERR("QoS Cubes configuration failed");

	return result;
}

bool DIFConfigValidator::knownIPCProcessAddresses()
{
	std::list<rina::StaticIPCProcessAddress> staticAddress =
			dif_config_.nsm_configuration_.
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
		LOG_ERR("Know IPCP Processes Addresses configuration failed");

	return result;
}

/*
bool DIFConfigValidator::pdufTableGeneratorConfiguration()
{
	bool result =
		dif_config_.pduft_generator_configuration_.
		pduft_generator_policy_.name_.compare("LinkState") == 0 &&
		dif_config_.pduft_generator_configuration_.
		pduft_generator_policy_.version_.compare("0") == 0;
	if (!result)
		LOG_ERR("PDUFT Generator configuration failed");

	return result;
}
*/

} //rinad namespace
