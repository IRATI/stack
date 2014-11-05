/*
 * IPC Manager
 *
 *    Vincenzo Maffione     <v.maffione@nextworks.it>
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <cstdlib>
#include <iostream>
#include <map>
#include <vector>

#include <librina/common.h>
#include <librina/ipc-manager.h>

#include "event-loop.h"
#include "rina-configuration.h"
#include "helpers.h"
#include "ipcm.h"
#include "console.h"
#include "registration.h"
#include "flow-allocation.h"

using namespace std;

namespace rinad {

#define IPCM_LOG_FILE "/tmp/ipcm-log-file"

void *
script_function(void *opaque)
{
        IPCManager *ipcm = static_cast<IPCManager *>(opaque);

        LOG_DBG("script starts");

        ipcm->apply_configuration();

        LOG_DBG("script stops");

        return NULL;
}

IPCManager::IPCManager(unsigned int wait_time) :
        concurrency(wait_time), script(NULL), console(NULL)
{ }

IPCManager::~IPCManager()
{
        if (console) {
                delete console;
        }

        if (script) {
                // Maybe we should join here
                delete script;
        }
}

void IPCManager::init(const std::string& logfile, const std::string& loglevel)
{
        // Initialize the IPC manager infrastructure in librina.
        try {
                rina::initializeIPCManager(1, config.local.installationPath,
                                config.local.libraryPath,
                                loglevel, logfile);
                LOG_DBG("IPC Manager daemon initialized");
                LOG_DBG("       installation path: %s",
                        config.local.installationPath.c_str());
                LOG_DBG("       library path: %s",
                        config.local.libraryPath.c_str());
                LOG_DBG("       log file: %s", IPCM_LOG_FILE);
        } catch (rina::InitializationException) {
                LOG_ERR("Error while initializing librina-ipc-manager");
                exit(EXIT_FAILURE);
        }
}

int
IPCManager::start_script_worker()
{
        if (script)
                return -1;

        rina::ThreadAttributes ta;
        script = new rina::Thread(&ta, script_function, this);

        return 0;
}

int
IPCManager::start_console_worker()
{
        if (console) {
                return -1;
        }

        rina::ThreadAttributes ta;
        console = new IPCMConsole(*this, ta);

        return 0;
}

bool
IPCMConcurrency::wait_for_event(rina::IPCEventType ty, unsigned int seqnum,
                                int &result)
{
        bool arrived = false;

        event_waiting = true;
        event_ty      = ty;
        event_sn      = seqnum;
        event_result  = -1;

        try {
                timedwait(wait_time, 0);
                arrived = true;
        } catch (rina::ConcurrentException) {
                event_waiting = false;
                event_result = -1;
        }

        result = event_result;

        return arrived;
}

void
IPCMConcurrency::notify_event(rina::IPCEvent *event)
{
        if (event_waiting && event_ty == event->eventType
                        && (event_sn == 0 ||
                            event_sn == event->sequenceNumber)) {
                signal();
                event_waiting = false;
        }
}

static void
ipcm_pre_function(rina::IPCEvent *event, EventLoopData *opaque)
{
        DOWNCAST_DECL(opaque, IPCManager, ipcm);

        (void) event;
        ipcm->concurrency.lock();
}

static void
ipcm_post_function(rina::IPCEvent *event, EventLoopData *opaque)
{
        DOWNCAST_DECL(opaque, IPCManager, ipcm);

        ipcm->concurrency.notify_event(event);
        ipcm->concurrency.unlock();
}

rina::IPCProcess *
IPCManager::create_ipcp(const rina::ApplicationProcessNamingInformation& name,
                        const string& type)
{
        rina::IPCProcess *      ipcp = NULL;
        bool                    wait = false;
        ostringstream           ss;
        rina::IPCProcessFactory fact;
        std::list<std::string>  supportedDIFS;
        bool                    difCorrect = false;
        std::string             s;


        concurrency.lock();

        try {
                // Check that the AP name is not empty
                if (name.processName == std::string()) {
                        ss << "Cannot create IPC process with an "
                                "empty AP name" << endl;
                        FLUSH_LOG(ERR, ss);

                        throw rina::CreateIPCProcessException();
                }

                // Check if @type is one of the supported DIF types.
                supportedDIFS = fact.getSupportedIPCProcessTypes();
                for (std::list<std::string>::iterator it =
                             supportedDIFS.begin();
                     it != supportedDIFS.end();
                     ++it) {
                        if (type == *it)
                                difCorrect = true;

                        s.append(*it);
                        s.append(", ");
                }

                if (!difCorrect) {
                        ss << "difType parameter of DIF "
                           << name.toString()
                           << " is wrong, options are: "
                           << s;
                        FLUSH_LOG(ERR, ss);

                        throw rina::CreateIPCProcessException();
                }

                ipcp = rina::ipcProcessFactory->create(name,
                                                       type);
                if (type != rina::NORMAL_IPC_PROCESS) {
                        // Shim IPC processes are set as initialized
                        // immediately.
                        ipcp->setInitialized();
                } else {
                        // Normal IPC processes can be set as
                        // initialized only when the corresponding
                        // IPC process daemon is initialized, so we
                        // defer the operation.
                        pending_normal_ipcp_inits[ipcp->id] = ipcp;
                        wait = true;
                }
                ss << "IPC process " << name.toString() << " created "
                        "[id = " << ipcp->id << "]" << endl;
                FLUSH_LOG(INFO, ss);

                if (wait) {
                        int ret;
                        bool arrived = concurrency.wait_for_event(
                                        rina::IPC_PROCESS_DAEMON_INITIALIZED_EVENT,
                                        0, ret);

                        if (!arrived) {
                                ss << "Timed out while waiting for IPC process daemon"
                                        "to initialize" << endl;
                                FLUSH_LOG(ERR, ss);
                        }
                }

        } catch (rina::CreateIPCProcessException) {
                ss << "Failed to create IPC process '" <<
                        name.toString() << "' of type '" <<
                        type << "'" << endl;
                FLUSH_LOG(ERR, ss);
        }

        concurrency.unlock();

        return ipcp;
}

int
IPCManager::destroy_ipcp(unsigned int ipcp_id)
{
        ostringstream ss;

        try {
                rina::ipcProcessFactory->destroy(ipcp_id);
                ss << "IPC process destroyed [id = " << ipcp_id
                        << "]" << endl;
                FLUSH_LOG(INFO, ss);
        } catch (rina::DestroyIPCProcessException) {
                ss  << ": Error while destroying IPC "
                        "process with id " << ipcp_id << endl;
                FLUSH_LOG(ERR, ss);
                return -1;
        }

        return 0;
}

int
IPCManager::list_ipcps(std::ostream& os)
{
        const vector<rina::IPCProcess *>& ipcps =
                rina::ipcProcessFactory->listIPCProcesses();

        os << "Current IPC processes:" << endl;
        for (unsigned int i = 0; i < ipcps.size(); i++) {
                os << "    " << ipcps[i]->id << ": " <<
                        ipcps[i]->name.toString() << "\n";
        }

        return 0;
}

int
IPCManager::list_ipcp_types(std::ostream& os)
{
        const list<string>& types = rina::ipcProcessFactory->
                                        getSupportedIPCProcessTypes();

        os << "Supported IPC process types:" << endl;
        for (list<string>::const_iterator it = types.begin();
                                        it != types.end(); it++) {
                os << "    " << *it << endl;
        }

        return 0;
}

/// DIFValidator CLASS
DIFConfigValidator::DIFConfigValidator(const rina::DIFConfiguration &dif_config,
                const rina::DIFInformation &dif_info, std::string type)
                :dif_config_(dif_config), dif_info_(dif_info)
{
        if (type == "normal-ipc")
                        type_ = NORMAL;
        else if (type == "shim-dummy")
                        type_ = SHIM_DUMMY;
        else
                        type_ = SHIM_ETH;
}

bool DIFConfigValidator::validateConfigs()
{
        if (type_ == SHIM_ETH)
                return validateShimEth();
        else if(type_ == SHIM_DUMMY)
                return validateShimDummy();
        else
                return validateNormal();
}

bool DIFConfigValidator::validateShimEth()
{
        return validateBasicDIFConfigs() && validateConfigParameters();
}
bool DIFConfigValidator::validateShimDummy()
{
        return validateBasicDIFConfigs();
}
bool DIFConfigValidator::validateNormal()
{
        return  dataTransferConstants() && qosCubes() &&
                        knownIPCProcessAddresses() && pdufTableGeneratorConfiguration();
}

bool DIFConfigValidator::validateBasicDIFConfigs()
{
        return !dif_info_.dif_name_.processName.empty();
}

bool DIFConfigValidator::validateConfigParameters()
{
        for (std::list<rina::Parameter>::const_iterator it =
                     dif_config_.parameters_.begin();
             it != dif_config_.parameters_.end();
             ++it) {
                if (it->name.compare("interface-name") == 0) {
                        return true;
                }
        }

        return false;
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

int
IPCManager::assign_to_dif(rina::IPCProcess * ipcp,
                          const rina::ApplicationProcessNamingInformation &
                          dif_name)
{
        if (!ipcp)
                return -1;

        rinad::DIFProperties   dif_props;
        rina::DIFInformation   dif_info;
        rina::DIFConfiguration dif_config;
        ostringstream          ss;
        unsigned int           seqnum;
        bool                   arrived = true;
        bool                   found;
        int                    ret = -1;

        concurrency.lock();

        try {

                // Try to extract the DIF properties from the
                // configuration.
                found = config.lookup_dif_properties(dif_name,
                                dif_props);
                if (!found) {
                        ss << "Cannot find properties for DIF "
                                << dif_name.toString();
                        throw Exception();
                }

                // Fill in the DIFConfiguration object.
                if (ipcp->type == rina::NORMAL_IPC_PROCESS) {
                        rina::EFCPConfiguration efcp_config;
                        rina::NamespaceManagerConfiguration nsm_config;
                        rina::AddressingConfiguration address_config;
                        unsigned int address;

                        // FIll in the EFCPConfiguration object.
                        efcp_config.set_data_transfer_constants(
                                        dif_props.dataTransferConstants);
                        rina::QoSCube * qosCube = 0;
                        for (list<rina::QoSCube>::iterator
                                        qit = dif_props.qosCubes.begin();
                                        qit != dif_props.qosCubes.end();
                                        qit++) {
                                qosCube = new rina::QoSCube(*qit);
                                efcp_config.add_qos_cube(qosCube);
                        }

                        for (list<AddressPrefixConfiguration>::iterator
                                        ait = dif_props.addressPrefixes.begin();
                                        ait != dif_props.addressPrefixes.end();
                                        ait ++) {
                                rina::AddressPrefixConfiguration prefix;
                                prefix.address_prefix_ = ait->addressPrefix;
                                prefix.organization_ = ait->organization;
                                address_config.address_prefixes_.push_back(prefix);
                        }

                        for (list<rinad::KnownIPCProcessAddress>::iterator
                                        kit = dif_props.knownIPCProcessAddresses.begin();
                                        kit != dif_props.knownIPCProcessAddresses.end();
                                        kit ++) {
                                rina::StaticIPCProcessAddress static_address;
                                static_address.ap_name_ = kit->name.processName;
                                static_address.ap_instance_ = kit->name.processInstance;
                                static_address.address_ = kit->address;
                                address_config.static_address_.push_back(static_address);
                        }
                        nsm_config.addressing_configuration_ = address_config;

                        found = dif_props.
                                lookup_ipcp_address(ipcp->name,
                                                address);
                        if (!found) {
                                ss << "No address for IPC process " <<
                                        ipcp->name.toString() <<
                                        " in DIF " << dif_name.toString() <<
                                        endl;
                                throw Exception();
                        }
                        dif_config.set_efcp_configuration(efcp_config);
                        dif_config.nsm_configuration_ = nsm_config;
                        dif_config.pduft_generator_configuration_ =
                                dif_props.pdufTableGeneratorConfiguration;
                        dif_config.rmt_configuration_ = dif_props.rmtConfiguration;
                        dif_config.set_address(address);
                }

                for (map<string, string>::const_iterator
                                pit = dif_props.configParameters.begin();
                                pit != dif_props.configParameters.end();
                                pit++) {
                        dif_config.add_parameter
                                (rina::Parameter(pit->first, pit->second));
                }

                // Fill in the DIFInformation object.
                dif_info.set_dif_name(dif_name);
                dif_info.set_dif_type(ipcp->type);
                dif_info.set_dif_configuration(dif_config);

                // Validate the parameters
                DIFConfigValidator validator(dif_config, dif_info,
                                ipcp->type);
                if(!validator.validateConfigs())
                        throw rina::BadConfigurationException("DIF configuration validator failed");

                // Invoke librina to assign the IPC process to the
                // DIF specified by dif_info.
                seqnum = ipcp->assignToDIF(dif_info);

                pending_ipcp_dif_assignments[seqnum] = ipcp;
                ss << "Requested DIF assignment of IPC process " <<
                        ipcp->name.toString() << " to DIF " <<
                        dif_name.toString() << endl;
                FLUSH_LOG(INFO, ss);
                arrived = concurrency.wait_for_event(rina::ASSIGN_TO_DIF_RESPONSE_EVENT,
                                                     seqnum, ret);
        } catch (rina::AssignToDIFException) {
                ss << "Error while assigning " <<
                        ipcp->name.toString() <<
                        " to DIF " << dif_name.toString() << endl;
                FLUSH_LOG(ERR, ss);
        } catch (rina::BadConfigurationException &e) {
                LOG_ERR("DIF %s configuration failed", dif_name.toString().c_str());
                throw e;
        }
        catch (Exception) {
                FLUSH_LOG(ERR, ss);
        }

        concurrency.unlock();

        if (!arrived) {
                ss  << ": Timed out" << endl;
                FLUSH_LOG(ERR, ss);
                return -1;
        }

        return ret;
}

int
IPCManager::register_at_dif(rina::IPCProcess *ipcp,
                            const rina::ApplicationProcessNamingInformation&
                            dif_name)
{
        // Select a slave (N-1) IPC process.
        rina::IPCProcess *slave_ipcp = select_ipcp_by_dif(dif_name);
        ostringstream ss;
        unsigned int seqnum;
        bool arrived = true;
        int ret = -1;

        if (!slave_ipcp) {
                ss << "Cannot find any IPC process belonging "
                   << "to DIF " << dif_name.toString()
                   << endl;
                FLUSH_LOG(ERR, ss);
                return -1;
        }

        concurrency.lock();

        // Try to register @ipcp to the slave IPC process.
        try {
                seqnum = slave_ipcp->registerApplication(
                                ipcp->name, ipcp->id);

                pending_ipcp_registrations[seqnum] =
                        make_pair(ipcp, slave_ipcp);

                ss << "Requested DIF registration of IPC process " <<
                        ipcp->name.toString() << " at DIF " <<
                        dif_name.toString() << " through IPC process "
                   << slave_ipcp->name.toString()
                   << endl;
                FLUSH_LOG(INFO, ss);

                arrived = concurrency.wait_for_event(
                        rina::IPCM_REGISTER_APP_RESPONSE_EVENT, seqnum, ret);
        } catch (Exception) {
                ss  << ": Error while requesting registration"
                    << endl;
                FLUSH_LOG(ERR, ss);
        }

        concurrency.unlock();

        if (!arrived) {
                ss  << ": Timed out" << endl;
                FLUSH_LOG(ERR, ss);
                return -1;
        }

        return ret;
}

int IPCManager::register_at_difs(rina::IPCProcess *ipcp,
                const list<rina::ApplicationProcessNamingInformation>& difs)
{
        for (list<rina::ApplicationProcessNamingInformation>::const_iterator
                        nit = difs.begin(); nit != difs.end(); nit++) {
                register_at_dif(ipcp, *nit);
        }

        return 0;
}

int
IPCManager::enroll_to_dif(rina::IPCProcess *ipcp,
                          const rinad::NeighborData& neighbor,
                          bool sync)
{
        ostringstream ss;
        bool arrived = true;
        int ret = -1;

        concurrency.lock();

        try {
                unsigned int seqnum;

                seqnum = ipcp->enroll(neighbor.difName,
                                neighbor.supportingDifName,
                                neighbor.apName);
                pending_ipcp_enrollments[seqnum] = ipcp;
                ss << "Requested enrollment of IPC process " <<
                        ipcp->name.toString() << " to DIF " <<
                        neighbor.difName.toString() << " through DIF "
                        << neighbor.supportingDifName.toString() <<
                        " and neighbor IPC process " <<
                        neighbor.apName.toString() << endl;
                FLUSH_LOG(INFO, ss);
                if (sync) {
                        arrived =
                                concurrency.wait_for_event(rina::ENROLL_TO_DIF_RESPONSE_EVENT,
                                                           seqnum,
                                                           ret);
                } else {
                        ret = 0;
                }
        } catch (rina::EnrollException) {
                ss  << ": Error while enrolling "
                        << "to DIF " << neighbor.difName.toString()
                        << endl;
                FLUSH_LOG(ERR, ss);
        }

        concurrency.unlock();

        if (!arrived) {
                ss  << ": Timed out" << endl;
                FLUSH_LOG(ERR, ss);
                return -1;
        }

        return ret;
}

int IPCManager::enroll_to_difs(rina::IPCProcess *ipcp,
                               const list<rinad::NeighborData>& neighbors)
{
        for (list<rinad::NeighborData>::const_iterator
                        nit = neighbors.begin();
                                nit != neighbors.end(); nit++) {
                enroll_to_dif(ipcp, *nit, false);
        }

        return 0;
}

int
IPCManager::apply_configuration()
{
        list<rina::IPCProcess *> ipcps;
        list<rinad::IPCProcessToCreate>::iterator cit;
        list<rina::IPCProcess *>::iterator pit;

        // Examine all the IPCProcesses that are going to be created
        // according to the configuration file.
        for (cit = config.ipcProcessesToCreate.begin();
             cit != config.ipcProcessesToCreate.end(); cit++) {
                rina::IPCProcess * ipcp = NULL;
                std::string        type;
                ostringstream      ss;

                if (!config.lookup_type_by_dif(cit->difName, type)) {
                        ss << "Failed to retrieve DIF type for "
                           << cit->name.toString() << endl;
                        FLUSH_LOG(ERR, ss);

                        continue;
                }

                try {
                        ipcp = create_ipcp(cit->name, type);
                        if (!ipcp) {
                                continue;
                        }
                        assign_to_dif(ipcp, cit->difName);
                        register_at_difs(ipcp, cit->difsToRegisterAt);
                } catch (Exception &e) {
                        LOG_ERR("Exception while applying configuration: %s",
                                e.what());
                }

                ipcps.push_back(ipcp);
        }

        // Perform all the enrollments specified by the configuration file.
        for (pit = ipcps.begin(), cit = config.ipcProcessesToCreate.begin();
             pit != ipcps.end();
             pit++, cit++) {
                enroll_to_difs(*pit, cit->neighbors);
        }

        return 0;
}

int
IPCManager::update_dif_configuration(rina::IPCProcess *             ipcp,
                                     const rina::DIFConfiguration & dif_config)
{
        ostringstream ss;
        bool arrived = true;
        int ret = -1;

        concurrency.lock();

        try {
                unsigned int seqnum;

                // Request a configuration update for the IPC process
                /* FIXME The meaning of this operation is unclear: what
                 * configuration is modified? The configuration of the
                 * IPC process only or the configuration of the whole DIF
                 * (which possibly contains more IPC process, both on the same
                 * processing systems and on different processing systems) ?
                 */
                seqnum = ipcp->updateDIFConfiguration(dif_config);
                pending_dif_config_updates[seqnum] = ipcp;

                ss << "Requested configuration update for IPC process " <<
                        ipcp->name.toString() << endl;
                FLUSH_LOG(INFO, ss);

                arrived = concurrency.wait_for_event(
                        rina::UPDATE_DIF_CONFIG_RESPONSE_EVENT, seqnum, ret);
        } catch (rina::UpdateDIFConfigurationException) {
                ss  << ": Error while updating DIF configuration "
                        " for IPC process " << ipcp->name.toString() << endl;
                FLUSH_LOG(ERR, ss);
        }

        concurrency.unlock();

        if (!arrived) {
                ss  << ": Timed out" << endl;
                FLUSH_LOG(ERR, ss);
                return -1;
        }

        return ret;
}

std::string
IPCManager::query_rib(rina::IPCProcess *ipcp)
{
        if (!ipcp) {
                return "Bogus input parameters";
        }

        std::string   retstr = "Query RIB operation was not successful";
        ostringstream ss;
        bool          arrived = true;
        int           ret = -1;

        concurrency.lock();

        // Invoke librina to assign the IPC process to the
        // DIF specified by dif_info.

        try {
                unsigned int seqnum = ipcp->queryRIB("", "", 0, 0, "");

                pending_ipcp_query_rib_responses[seqnum] = ipcp;
                ss << "Requested query RIB of IPC process " <<
                        ipcp->name.toString() << endl;
                FLUSH_LOG(INFO, ss);
                arrived = concurrency.wait_for_event(rina::QUERY_RIB_RESPONSE_EVENT,
                                           seqnum, ret);

                std::map<unsigned int, std::string >::iterator mit;
                mit = query_rib_responses.find(seqnum);
                if (mit != query_rib_responses.end()) {
                        retstr = mit->second;
                        query_rib_responses.erase(seqnum);
                }

        } catch (rina::QueryRIBException) {
                ss << "Error while querying RIB of IPC Process " <<
                        ipcp->name.toString() << endl;
                FLUSH_LOG(ERR, ss);
        }

        concurrency.unlock();

        if (!arrived) {
                ss  << ": Timed out" << endl;
                FLUSH_LOG(ERR, ss);
        }

        return retstr;
}

static void
application_unregistered_event_handler(rina::IPCEvent * event,
                                       EventLoopData *  opaque)
{
        (void) event;  // Stop compiler barfs
        (void) opaque; // Stop compiler barfs
}

static void
assign_to_dif_request_event_handler(rina::IPCEvent * event,
                                    EventLoopData *  opaque)
{
        (void) event;  // Stop compiler barfs
        (void) opaque; // Stop compiler barfs
}

static void
assign_to_dif_response_event_handler(rina::IPCEvent *       e,
                                            EventLoopData * opaque)
{
        DOWNCAST_DECL(e, rina::AssignToDIFResponseEvent, event);
        DOWNCAST_DECL(opaque, IPCManager, ipcm);
        map<unsigned int, rina::IPCProcess*>::iterator mit;
        ostringstream ss;
        bool success = (event->result == 0);
        int ret = -1;

        mit = ipcm->pending_ipcp_dif_assignments.find(
                                        event->sequenceNumber);
        if (mit != ipcm->pending_ipcp_dif_assignments.end()) {
                rina::IPCProcess *ipcp = mit->second;

                // Inform the IPC process about the result of the
                // DIF assignment operation
                try {
                        ipcp->assignToDIFResult(success);
                        ss << "DIF assignment operation completed for IPC "
                                << "process " << ipcp->name.toString() <<
                                " [success=" << success << "]" << endl;
                        FLUSH_LOG(INFO, ss);
                        ret = 0;
                } catch (rina::AssignToDIFException) {
                        ss << ": Error while reporting DIF "
                                "assignment result for IPC process "
                                << ipcp->name.toString() << endl;
                        FLUSH_LOG(ERR, ss);
                }
                ipcm->pending_ipcp_dif_assignments.erase(mit);
        } else {
                ss << ": Warning: DIF assignment response "
                        "received, but no pending DIF assignment" << endl;
                FLUSH_LOG(WARN, ss);
        }

        ipcm->concurrency.set_event_result(ret);
}

static void
update_dif_config_request_event_handler(rina::IPCEvent *event,
                                        EventLoopData *opaque)
{
        (void) event;  // Stop compiler barfs
        (void) opaque; // Stop compiler barfs
}

static void
update_dif_config_response_event_handler(rina::IPCEvent *e,
                                         EventLoopData *opaque)
{
        DOWNCAST_DECL(e, rina::UpdateDIFConfigurationResponseEvent, event);
        DOWNCAST_DECL(opaque, IPCManager, ipcm);
        map<unsigned int, rina::IPCProcess*>::iterator mit;
        bool success = (event->result == 0);
        rina::IPCProcess *ipcp = NULL;
        ostringstream ss;

        mit = ipcm->pending_dif_config_updates.find(event->sequenceNumber);
        if (mit == ipcm->pending_dif_config_updates.end()) {
                ss  << ": Warning: DIF configuration update "
                        "response received, but no corresponding pending "
                        "request" << endl;
                FLUSH_LOG(WARN, ss);
                return;
        }

        ipcp = mit->second;
        try {

                // Inform the requesting IPC process about the result of
                // the configuration update operation
                ipcp->updateDIFConfigurationResult(success);
                ss << "Configuration update operation completed for IPC "
                        << "process " << ipcp->name.toString() <<
                        " [success=" << success << "]" << endl;
                FLUSH_LOG(INFO, ss);
        } catch (rina::UpdateDIFConfigurationException) {
                ss  << ": Error while reporting DIF "
                        "configuration update for process " <<
                        ipcp->name.toString() << endl;
                FLUSH_LOG(ERR, ss);
        }

        ipcm->pending_dif_config_updates.erase(mit);

        ipcm->concurrency.set_event_result(event->result);
}

static void
enroll_to_dif_request_event_handler(rina::IPCEvent *event, EventLoopData *opaque)
{
        (void) event; // Stop compiler barfs
        (void) opaque;    // Stop compiler barfs
}

static void
enroll_to_dif_response_event_handler(rina::IPCEvent *e,
                                                 EventLoopData *opaque)
{
        DOWNCAST_DECL(e, rina::EnrollToDIFResponseEvent, event);
        DOWNCAST_DECL(opaque, IPCManager, ipcm);
        map<unsigned int, rina::IPCProcess *>::iterator mit;
        rina::IPCProcess *ipcp = NULL;
        bool success = (event->result == 0);
        ostringstream ss;
        int ret = -1;

        mit = ipcm->pending_ipcp_enrollments.find(event->sequenceNumber);
        if (mit == ipcm->pending_ipcp_enrollments.end()) {
                ss  << ": Warning: IPC process enrollment "
                        "response received, but no corresponding pending "
                        "request" << endl;
                FLUSH_LOG(WARN, ss);
        } else {
                ipcp = mit->second;
                if (success) {
                        ipcp->addNeighbors(event->neighbors);
                        ipcp->setDIFInformation(event->difInformation);
                        ss << "Enrollment operation completed for IPC "
                                << "process " << ipcp->name.toString() << endl;
                        FLUSH_LOG(INFO, ss);
                        ret = 0;
                } else {
                        ss  << ": Error: Enrollment operation of "
                                "process " << ipcp->name.toString() << " failed"
                                << endl;
                        FLUSH_LOG(ERR, ss);
                }

                ipcm->pending_ipcp_enrollments.erase(mit);
        }

        ipcm->concurrency.set_event_result(ret);
}

static void
neighbors_modified_notification_event_handler(rina::IPCEvent * e,
                                              EventLoopData *  opaque)
{
        DOWNCAST_DECL(e, rina::NeighborsModifiedNotificationEvent, event);
        DOWNCAST_DECL(opaque, IPCManager, ipcm);

        rina::IPCProcess *ipcp =
                rina::ipcProcessFactory->
                getIPCProcess(event->ipcProcessId);
        ostringstream ss;

        if (!event->neighbors.size()) {
                ss  << ": Warning: Empty neighbors-modified "
                        "notification received" << endl;
                FLUSH_LOG(WARN, ss);
                return;
        }

        if (!ipcp) {
                ss  << ": Error: IPC process unexpectedly "
                        "went away" << endl;
                FLUSH_LOG(ERR, ss);
                return;
        }

        if (event->added) {
                // We have new neighbors
                ipcp->addNeighbors(event->neighbors);
        } else {
                // We have lost some neighbors
                ipcp->removeNeighbors(event->neighbors);
        }
        ss << "Neighbors update [" << (event->added ? "+" : "-") <<
                "#" << event->neighbors.size() << "]for IPC process " <<
                ipcp->name.toString() <<  endl;
        FLUSH_LOG(INFO, ss);

        (void) ipcm;
}

static void
ipc_process_dif_registration_notification_handler(rina::IPCEvent *event, EventLoopData *opaque)
{
        (void) event;  // Stop compiler barfs
        (void) opaque; // Stop compiler barfs
}

static void
ipc_process_query_rib_handler(rina::IPCEvent *event, EventLoopData *opaque)
{
        (void) event;  // Stop compiler barfs
        (void) opaque; // Stop compiler barfs
}

static void
get_dif_properties_handler(rina::IPCEvent *event, EventLoopData *opaque)
{
        (void) event;  // Stop compiler barfs
        (void) opaque; // Stop compiler barfs
}

static void
get_dif_properties_response_event_handler(rina::IPCEvent *event, EventLoopData *opaque)
{
        (void) event;  // Stop compiler barfs
        (void) opaque; // Stop compiler barfs
}

static void
os_process_finalized_handler(rina::IPCEvent *e,
                                         EventLoopData *opaque)
{
        DOWNCAST_DECL(e, rina::OSProcessFinalizedEvent, event);
        DOWNCAST_DECL(opaque, IPCManager, ipcm);
        const vector<rina::IPCProcess *>& ipcps =
                rina::ipcProcessFactory->listIPCProcesses();
        const rina::ApplicationProcessNamingInformation& app_name =
                                                event->applicationName;
        list<rina::FlowInformation> involved_flows;
        ostringstream ss;

        ss  << "Application " << app_name.toString()
                        << "terminated" << endl;
        FLUSH_LOG(INFO, ss);

        // Look if the terminating application has allocated flows
        // with some IPC processes
        collect_flows_by_application(app_name, involved_flows);
        for (list<rina::FlowInformation>::iterator fit = involved_flows.begin();
                        fit != involved_flows.end(); fit++) {
                rina::IPCProcess *ipcp = select_ipcp_by_dif(fit->difName);
                rina::FlowDeallocateRequestEvent req_event(fit->portId, 0);

                if (!ipcp) {
                        ss  << ": Cannot find the IPC process "
                                "that provides the flow with port-id " <<
                                fit->portId << endl;
                        FLUSH_LOG(ERR, ss);
                        continue;
                }

                ipcm->deallocate_flow(ipcp, req_event);
        }

        // Look if the terminating application has pending registrations
        // with some IPC processes
        for (unsigned int i = 0; i < ipcps.size(); i++) {
                if (application_is_registered_to_ipcp(app_name,
                                                            ipcps[i])) {
                        // Build a structure that will be used during
                        // the unregistration process. The last argument
                        // is the request sequence number: 0 means that
                        // the unregistration response does not match
                        // an application request - this is indeed an
                        // unregistration forced by the IPCM.
                        rina::ApplicationUnregistrationRequestEvent
                                req_event(app_name, ipcps[i]->
                                        getDIFInformation().dif_name_, 0);

                        ipcm->unregister_app_from_ipcp(req_event, ipcps[i]);
                }
        }

        if (event->ipcProcessId != 0) {
                // TODO The process that crashed was an IPC Process daemon
                // Should we destroy the state in the kernel? Or try to
                // create another IPC Process in user space to bring it back?
        }
}

static void
query_rib_response_event_handler(rina::IPCEvent *e,
                                             EventLoopData *opaque)
{
        DOWNCAST_DECL(e, rina::QueryRIBResponseEvent, event);
        DOWNCAST_DECL(opaque, IPCManager, ipcm);
        ostringstream ss;
        map<unsigned int, rina::IPCProcess *>::iterator mit;
        rina::IPCProcess *ipcp = NULL;
        bool success = (event->result == 0);
        int ret = -1;

        ss << "Query RIB response event arrived" << endl;
        FLUSH_LOG(INFO, ss);

        mit = ipcm->pending_ipcp_query_rib_responses.find(event->sequenceNumber);
        if (mit == ipcm->pending_ipcp_query_rib_responses.end()) {
                ss  << ": Warning: IPC process query RIB "
                        "response received, but no corresponding pending "
                        "request" << endl;
                FLUSH_LOG(WARN, ss);
        } else {
                ipcp = mit->second;
                if (success) {
                        std::stringstream ss;
                        list<rina::RIBObjectData>::iterator lit;

                        ss << "Query RIB operation completed for IPC "
                                << "process " << ipcp->name.toString() << endl;
                        FLUSH_LOG(INFO, ss);
                        for (lit = event->ribObjects.begin(); lit != event->ribObjects.end();
                                        ++lit) {
                                ss << "Name: " << lit->name_ <<
                                        "; Class: "<< lit->class_;
                                ss << "; Instance: "<< lit->instance_ << endl;
                                ss << "Value: " << lit->displayable_value_ <<endl;
                                ss << "" << endl;
                        }
                        ipcm->query_rib_responses[event->sequenceNumber] = ss.str();
                        ret = 0;
                } else {
                        ss  << ": Error: Query RIB operation of "
                                "process " << ipcp->name.toString() << " failed"
                                << endl;
                        FLUSH_LOG(ERR, ss);
                }

                ipcm->pending_ipcp_query_rib_responses.erase(mit);
        }

        ipcm->concurrency.set_event_result(ret);
}

static void
ipc_process_daemon_initialized_event_handler(rina::IPCEvent * e,
                                             EventLoopData *  opaque)
{
        DOWNCAST_DECL(e, rina::IPCProcessDaemonInitializedEvent, event);
        DOWNCAST_DECL(opaque, IPCManager, ipcm);
        map<unsigned short, rina::IPCProcess *>::iterator mit;
        ostringstream ss;
        int ret = -1;

        // Perform deferred "setInitiatialized()" of a normal IPC process, if
        // needed.
        mit = ipcm->pending_normal_ipcp_inits.find(event->ipcProcessId);
        if (mit != ipcm->pending_normal_ipcp_inits.end()) {
                mit->second->setInitialized();
                ipcm->pending_normal_ipcp_inits.erase(mit);
                ss << "IPC process daemon initialized [id = " <<
                        event->ipcProcessId<< "]" << endl;
                FLUSH_LOG(INFO, ss);
                ret = 0;
        } else {
                ss << ": Warning: IPCP daemon initialized, "
                        "but no pending normal IPC process initialization"
                        << endl;
                FLUSH_LOG(WARN, ss);
        }

        ipcm->concurrency.set_event_result(ret);
}

static void
timer_expired_event_handler(rina::IPCEvent *event, EventLoopData *opaque)
{
        (void) event;  // Stop compiler barfs
        (void) opaque; // Stop compiler barfs
}

static void
ipc_process_create_connection_response_handler(rina::IPCEvent * event,
                                               EventLoopData *  opaque)
{
        (void) event;  // Stop compiler barfs
        (void) opaque; // Stop compiler barfs
}

static void
ipc_process_update_connection_response_handler(rina::IPCEvent * event,
                                               EventLoopData *  opaque)
{
        (void) event;  // Stop compiler barfs
        (void) opaque; // Stop compiler barfs
}

static void
ipc_process_create_connection_result_handler(rina::IPCEvent * event,
                                             EventLoopData *  opaque)
{
        (void) event;  // Stop compiler barfs
        (void) opaque; // Stop compiler barfs
}

static void
ipc_process_destroy_connection_result_handler(rina::IPCEvent * event,
                                              EventLoopData *  opaque)
{
        (void) event;  // Stop compiler barfs
        (void) opaque; // Stop compiler barfs
}

static void
ipc_process_dump_ft_response_handler(rina::IPCEvent * event,
                                     EventLoopData *  opaque)
{
        (void) event;  // Stop compiler barfs
        (void) opaque; // Stop compiler barfs
}


void
register_handlers_all(EventLoop& loop)
{
        loop.register_pre_function(ipcm_pre_function);
        loop.register_post_function(ipcm_post_function);

        loop.register_event(rina::FLOW_ALLOCATION_REQUESTED_EVENT,
                        flow_allocation_requested_event_handler);
        loop.register_event(rina::ALLOCATE_FLOW_REQUEST_RESULT_EVENT,
                        allocate_flow_request_result_event_handler);
        loop.register_event(rina::ALLOCATE_FLOW_RESPONSE_EVENT,
                        allocate_flow_response_event_handler);
        loop.register_event(rina::FLOW_DEALLOCATION_REQUESTED_EVENT,
                        flow_deallocation_requested_event_handler);
        loop.register_event(rina::DEALLOCATE_FLOW_RESPONSE_EVENT,
                        deallocate_flow_response_event_handler);
        loop.register_event(rina::APPLICATION_UNREGISTERED_EVENT,
                        application_unregistered_event_handler);
        loop.register_event(rina::FLOW_DEALLOCATED_EVENT,
                        flow_deallocated_event_handler);
        loop.register_event(rina::APPLICATION_REGISTRATION_REQUEST_EVENT,
                        application_registration_request_event_handler);
        loop.register_event(rina::REGISTER_APPLICATION_RESPONSE_EVENT,
                        register_application_response_event_handler);
        loop.register_event(rina::APPLICATION_UNREGISTRATION_REQUEST_EVENT,
                        application_unregistration_request_event_handler);
        loop.register_event(rina::UNREGISTER_APPLICATION_RESPONSE_EVENT,
                        unregister_application_response_event_handler);
        loop.register_event(rina::APPLICATION_REGISTRATION_CANCELED_EVENT,
                        application_registration_canceled_event_handler);
        loop.register_event(rina::ASSIGN_TO_DIF_REQUEST_EVENT,
                        assign_to_dif_request_event_handler);
        loop.register_event(rina::ASSIGN_TO_DIF_RESPONSE_EVENT,
                        assign_to_dif_response_event_handler);
        loop.register_event(rina::UPDATE_DIF_CONFIG_REQUEST_EVENT,
                        update_dif_config_request_event_handler);
        loop.register_event(rina::UPDATE_DIF_CONFIG_RESPONSE_EVENT,
                        update_dif_config_response_event_handler);
        loop.register_event(rina::ENROLL_TO_DIF_REQUEST_EVENT,
                        enroll_to_dif_request_event_handler);
        loop.register_event(rina::ENROLL_TO_DIF_RESPONSE_EVENT,
                        enroll_to_dif_response_event_handler);
        loop.register_event(rina::NEIGHBORS_MODIFIED_NOTIFICATION_EVENT,
                        neighbors_modified_notification_event_handler);
        loop.register_event(rina::IPC_PROCESS_DIF_REGISTRATION_NOTIFICATION,
                        ipc_process_dif_registration_notification_handler);
        loop.register_event(rina::IPC_PROCESS_QUERY_RIB,
                        ipc_process_query_rib_handler);
        loop.register_event(rina::GET_DIF_PROPERTIES,
                        get_dif_properties_handler);
        loop.register_event(rina::GET_DIF_PROPERTIES_RESPONSE_EVENT,
                        get_dif_properties_response_event_handler);
        loop.register_event(rina::OS_PROCESS_FINALIZED,
                        os_process_finalized_handler);
        loop.register_event(rina::IPCM_REGISTER_APP_RESPONSE_EVENT,
                        ipcm_register_app_response_event_handler);
        loop.register_event(rina::IPCM_UNREGISTER_APP_RESPONSE_EVENT,
                        ipcm_unregister_app_response_event_handler);
        loop.register_event(rina::IPCM_DEALLOCATE_FLOW_RESPONSE_EVENT,
                        ipcm_deallocate_flow_response_event_handler);
        loop.register_event(rina::IPCM_ALLOCATE_FLOW_REQUEST_RESULT,
                        ipcm_allocate_flow_request_result_handler);
        loop.register_event(rina::QUERY_RIB_RESPONSE_EVENT,
                        query_rib_response_event_handler);
        loop.register_event(rina::IPC_PROCESS_DAEMON_INITIALIZED_EVENT,
                        ipc_process_daemon_initialized_event_handler);
        loop.register_event(rina::TIMER_EXPIRED_EVENT,
                        timer_expired_event_handler);
        loop.register_event(rina::IPC_PROCESS_CREATE_CONNECTION_RESPONSE,
                        ipc_process_create_connection_response_handler);
        loop.register_event(rina::IPC_PROCESS_UPDATE_CONNECTION_RESPONSE,
                        ipc_process_update_connection_response_handler);
        loop.register_event(rina::IPC_PROCESS_CREATE_CONNECTION_RESULT,
                        ipc_process_create_connection_result_handler);
        loop.register_event(rina::IPC_PROCESS_DESTROY_CONNECTION_RESULT,
                        ipc_process_destroy_connection_result_handler);
        loop.register_event(rina::IPC_PROCESS_DUMP_FT_RESPONSE,
                        ipc_process_dump_ft_response_handler);
}

}
