/*
 * DIF Template Manager
 *
 *    Eduard Grasa     <eduard.grasa@i2cat.net>
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

#include <iostream>
#include <fstream>
#include <dirent.h>
#include <errno.h>
#include <poll.h>
#include <sys/inotify.h>

#define RINA_PREFIX     "dif-template-manager"
#include <librina/logs.h>
#include <librina/json/json.h>

#include "dif-template-manager.h"

using namespace std;

//Class DIFConfigFolderMonitor
DIFConfigFolderMonitor::DIFConfigFolderMonitor(const std::string& folder,
		   	   	       	       DIFTemplateManager * dtm) :
		rina::SimpleThread(std::string("dif-config-folder-monitor"), false)
{
	folder_name = folder;
	stop = false;
	dif_template_manager = dtm;
}

DIFConfigFolderMonitor::~DIFConfigFolderMonitor() throw()
{
}

void DIFConfigFolderMonitor::do_stop()
{
	rina::ScopedLock g(lock);
	stop = true;
}

bool DIFConfigFolderMonitor::has_to_stop()
{
	rina::ScopedLock g(lock);
	return stop;
}

static bool str_ends_with(const std::string& str, const std::string& suffix)
{
	if (suffix.size() > str.size()) {
		return false;
	}

	return std::equal(suffix.rbegin(), suffix.rend(), str.rbegin());
}

void DIFConfigFolderMonitor::process_events(int fd)
{
        char buf[4096]
            __attribute__ ((aligned(__alignof__(struct inotify_event))));
        const struct inotify_event *event;
        ssize_t len;
        char *ptr;
        std::stringstream ss;
        rinad::DIFTemplate * dif_template;

        for (;;) {
                len = read(fd, buf, sizeof buf);
                if (len == -1 && errno != EAGAIN) {
                    LOG_ERR("Problems reading inotify file descriptor");
                    return;
                }

                if (len <= 0)
                    return;

                for (ptr = buf; ptr < buf + len;
                		ptr += sizeof(struct inotify_event) + event->len) {
                	event = (const struct inotify_event *) ptr;
                	std::string file_name = std::string(event->name);

			if (! str_ends_with(file_name, ".dif")) {
				continue;
			}

                	if (event->mask & IN_CLOSE_WRITE) {
                		LOG_DBG("The file of DIF template %s has been modified.",
                				event->name);

                		ss << folder_name << "/" << file_name;
                		dif_template = rinad::parse_dif_template(ss.str(), file_name);
                		ss.str(std::string());
                		if (dif_template != 0) {
                			//TODO augment dif_template with the defaults
                			dif_template_manager->add_dif_template(file_name, dif_template);
                		}
                	}

                	if (event->mask & IN_DELETE) {
                		LOG_DBG("The file of DIF template %s has been deleted.",
                				event->name);
                		dif_template_manager->remove_dif_template(file_name);
                	}
                }
        }
}

int DIFConfigFolderMonitor::run()
{
	int fd;
	int wd;
	int pollnum;
	struct pollfd fds[1];

	LOG_DBG("DIF Template monitor started, monitoring folder %s",
			folder_name.c_str());

	fd = inotify_init1(IN_NONBLOCK);
	if (fd == -1) {
		LOG_ERR("Error initializing inotify, stopping DIF template monitor");
		return -1;
	}

	wd = inotify_add_watch(fd, folder_name.c_str(), IN_CLOSE_WRITE | IN_DELETE);
	if (wd == -1) {
		LOG_ERR("Error adding a watch, stopping DIF template monitor");
		close(fd);
		return -1;
	}

	fds[0].fd = fd;
	fds[0].events = POLLIN;

	while (!has_to_stop()) {
		pollnum = poll(fds, 1, 1000);

		if (pollnum == EINVAL) {
			LOG_ERR("Poll returned EINVAL, stopping DIF template monitor");
			close(fd);
			return -1;
		}

		if (pollnum <= 0) {
			//No changes during this period or error that can be ignored
			continue;
		}

		if (fds[0].revents & POLLIN) {
			process_events(fd);
		}
	}

        /* Close inotify file descriptor */
        close(fd);

        LOG_DBG("DIF Template Manager stopped");

	return 0;
}

//Class DIF Template Manager
const std::string DIFTemplateManager::DEFAULT_TEMPLATE_NAME = "default.dif";

DIFTemplateManager::DIFTemplateManager(const std::string& folder)
{
	folder_name = folder;
	LOG_INFO("DIF Template folder: %s", folder_name.c_str());
	default_template = 0;

	//load current templates from template folder
	if (load_initial_dif_templates() != 0) {
		return;
	}

	//Create a thread that monitors the DIF template folder when required
	monitor = new DIFConfigFolderMonitor(folder_name,
					     this);
	monitor->start();
}

DIFTemplateManager::~DIFTemplateManager()
{
	void * status;

	if (monitor) {
		monitor->do_stop();
		monitor->join(&status);

		delete monitor;
	}

	// Destroy all DIF templates in the map
	for (std::map<std::string, rinad::DIFTemplate *>::iterator it = dif_templates.begin();
			it != dif_templates.end(); ++it){
		delete it->second;
	}
}

int DIFTemplateManager::load_initial_dif_templates()
{
	DIR *dirp;
	struct dirent *dp;
	rinad::DIFTemplate * dif_template;
	std::string file_name;
	std::stringstream ss;
	std::list<rinad::DIFTemplate *> templates;

	if ((dirp = opendir(folder_name.c_str())) == NULL) {
		LOG_ERR("Failed to open folder %s", folder_name.c_str());
		return -1;
	}

	do {
		errno = 0;
		if ((dp = readdir(dirp)) != NULL) {
			if (! str_ends_with(std::string(dp->d_name), ".dif")) {
				continue;
			}

			LOG_DBG("Found DIF template file called: %s", dp->d_name);
			file_name = std::string(dp->d_name);
			ss << folder_name << "/" << file_name;
			dif_template = rinad::parse_dif_template(ss.str(), file_name);
			ss.str(std::string());

			if (strcmp(dp->d_name, DEFAULT_TEMPLATE_NAME.c_str()) == 0) {
				LOG_DBG("Default DIF template found");
				default_template = dif_template;
				add_dif_template(file_name, dif_template);
			} else {
				templates.push_back(dif_template);
			}
		}
	} while (dp != NULL);

	closedir(dirp);

	//Add all templates, cannot be done before because first we need to add
	//the default template (if present)
	for (std::list<rinad::DIFTemplate *>::iterator it = templates.begin();
			it != templates.end(); ++it) {
		add_dif_template((*it)->templateName, *it);
	}

	if (!default_template) {
		LOG_WARN("Default DIF template not present");
	}

	return 0;
}

int DIFTemplateManager::get_dif_template(const std::string& name, rinad::DIFTemplate& dif_template)
{
	rina::ReadScopedLock g(templates_lock);

	std::map<std::string, rinad::DIFTemplate*>::iterator it = dif_templates.find(name);
	if (it == dif_templates.end()) {
		LOG_ERR("Could not find DIF template with name %s", name.c_str());
		return -1;
	} else {
		dif_template = *(it->second);
		return 0;
	}
}

void DIFTemplateManager::augment_dif_template(rinad::DIFTemplate * dif_template)
{
	if (!default_template || dif_template->templateName == DEFAULT_TEMPLATE_NAME ||
			dif_template->difType != rina::NORMAL_IPC_PROCESS) {
		return;
	}

	if (dif_template->dataTransferConstants.address_length_ == 0) {
		dif_template->dataTransferConstants = default_template->dataTransferConstants;
	}

	if (dif_template->qosCubes.size() == 0) {
		for (std::list<rina::QoSCube>::iterator it = default_template->qosCubes.begin();
				it != default_template->qosCubes.end(); ++it) {
			dif_template->qosCubes.push_back(rina::QoSCube(*it));
		}
	}

	if (dif_template->etConfiguration.policy_set_.name_ == std::string()) {
		dif_template->etConfiguration = default_template->etConfiguration;
	}

	if (dif_template->rmtConfiguration.pft_conf_.policy_set_.name_ == std::string()) {
		dif_template->rmtConfiguration.pft_conf_ = default_template->rmtConfiguration.pft_conf_;
	}

	if (dif_template->rmtConfiguration.policy_set_.name_ == std::string()) {
		dif_template->rmtConfiguration = default_template->rmtConfiguration;
	}

	if (dif_template->routingConfiguration.policy_set_.name_ == std::string()) {
		dif_template->routingConfiguration = default_template->routingConfiguration;
	}

	if (dif_template->nsmConfiguration.policy_set_.name_ == std::string()) {
		dif_template->nsmConfiguration = default_template->nsmConfiguration;
	}

	if (dif_template->faConfiguration.policy_set_.name_ == std::string()) {
		dif_template->faConfiguration = default_template->faConfiguration;
	}

	if (dif_template->raConfiguration.pduftg_conf_.policy_set_.name_ == std::string()) {
		dif_template->raConfiguration = default_template->raConfiguration;
	}

	if (dif_template->knownIPCProcessAddresses.size() == 0) {
		for (std::list<rinad::KnownIPCProcessAddress>::iterator it =
				default_template->knownIPCProcessAddresses.begin();
				it != default_template->knownIPCProcessAddresses.end(); ++it) {
			dif_template->knownIPCProcessAddresses.push_back(rinad::KnownIPCProcessAddress(*it));
		}
	}

	if (dif_template->addressPrefixes.size() == 0) {
		for (std::list<rinad::AddressPrefixConfiguration>::iterator it =
				default_template->addressPrefixes.begin();
				it != default_template->addressPrefixes.end(); ++it) {
			dif_template->addressPrefixes.push_back(rinad::AddressPrefixConfiguration(*it));
		}
	}

	if (dif_template->secManConfiguration.policy_set_.name_ == std::string()) {
		dif_template->secManConfiguration = default_template->secManConfiguration;
	}

	if (dif_template->configParameters.size() == 0 && default_template->configParameters.size() != 0) {
		for (std::map<std::string, std::string>::iterator it =
				default_template->configParameters.begin();
				it != default_template->configParameters.end(); ++it) {
			dif_template->configParameters[it->first] = it->second;
		}
	}
}

void DIFTemplateManager::add_dif_template(const std::string& name,
					  rinad::DIFTemplate * dif_template)
{
	if (!dif_template) {
		LOG_ERR("Cannot add a bogus dif_template");
		return;
	}

	rina::WriteScopedLock g(templates_lock);

	//Augment dif_template with default_template
	augment_dif_template(dif_template);

	//If the template already exists destroy the old version
	internal_remove_dif_template(name);
	dif_templates[name] = dif_template;

	if (name == DEFAULT_TEMPLATE_NAME) {
		default_template = dif_template;
	}

	LOG_INFO("Added or modified DIF template called: %s", name.c_str());
}

void DIFTemplateManager::remove_dif_template(const std::string& name)
{
	rina::WriteScopedLock g(templates_lock);

	internal_remove_dif_template(name);

	if (name == DEFAULT_TEMPLATE_NAME) {
		LOG_WARN("Default DIF template removed");
		default_template = 0;
	}
}

void DIFTemplateManager::internal_remove_dif_template(const std::string& name)
{
	std::map<std::string, rinad::DIFTemplate*>::iterator it = dif_templates.find(name);
	if (it != dif_templates.end()) {
		dif_templates.erase(name);
		delete it->second;
		LOG_INFO("Removed DIF template called: %s", name.c_str());
	}
}

void DIFTemplateManager::get_all_dif_templates(std::list<rinad::DIFTemplate>& out_templates)
{
	std::map<std::string, rinad::DIFTemplate*>::iterator it;
	rina::ReadScopedLock g(templates_lock);

	for (it = dif_templates.begin(); it != dif_templates.end(); ++it) {
		out_templates.push_back(*(it->second));
	}
}

int DIFTemplateManager::parse_dif_descriptor(const std::string& desc_file,
			 	 	     DIFDescriptor & dif_desc)
{
	Json::Value    root, ipcps, regs, neighbors;
	Json::Reader   reader;
	rina::Neighbor neigh;
	unsigned int   i, j;
	ifstream       file;
	IPCPDescriptor ipcp_desc;
	std::string    dif_type;

        file.open(desc_file.c_str(), std::ifstream::in);
        if (file.fail()) {
                LOG_ERR("Failed to open config file");
                return -1;
        }

        if (!reader.parse(file, root, false)) {
        	LOG_ERR("Failed to parse JSON: %s",
        		 reader.getFormatedErrorMessages().c_str());

        	return -1;
        }

        file.close();

        dif_desc.dif_name = root.get("dif_name", string()).asString();
        dif_type = root.get("dif_type", string()).asString();
        ipcps = root["ipcps"];
        if (ipcps == 0) {
        	LOG_ERR("DIF descriptor does not contain IPCP configs");
        	return -1;
        }

        for (i = 0; i < ipcps.size(); i++) {
        	ipcp_desc.dif_template_name = ipcps[i].get("dif_template", string()).asString();
        	ipcp_desc.system_name = ipcps[i].get("system_name", string()).asString();
        	ipcp_desc.ipcp_type = dif_type;
        	ipcp_desc.dif_name = dif_desc.dif_name;

        	regs = ipcps[i]["registrations"];
        	if (regs != 0) {
        		for (j = 0; j < regs.size(); j++) {
        			ipcp_desc.difs_to_register_at.push_back(regs[j].asString());
        		}
        	}

        	neighbors = ipcps[i]["neighbors"];
        	if (neighbors != 0) {
        		for (j = 0; j < neighbors.size(); j++) {
        			neigh.name_.processName = neighbors[j].get("ipcp_name", string()).asString();
        			neigh.name_.processInstance = "1";
        			neigh.supporting_dif_name_.processName = neighbors[j].get("under_dif", string()).asString();

        			ipcp_desc.neighbors.push_back(neigh);
        		}
        	}

        	dif_desc.ipcps.push_back(ipcp_desc);
        	ipcp_desc.difs_to_register_at.clear();
        	ipcp_desc.neighbors.clear();
        }

        return 0;
}

int DIFTemplateManager::parse_ipcp_descriptor(const std::string& desc_file,
			  	  	      IPCPDescriptor & ipcp_desc)
{
        // Parse config file with jsoncpp
        Json::Value  root, ipcp_data, difs;
        Json::Reader reader;
        ifstream     file;

        file.open(desc_file.c_str(), std::ifstream::in);
        if (file.fail()) {
                LOG_ERR("Failed to open config file");
                return -1;
        }

        if (!reader.parse(file, root, false)) {
        	LOG_ERR("Failed to parse JSON: %s",
        		 reader.getFormatedErrorMessages().c_str());

        	return -1;
        }

        file.close();

        ipcp_data = root["ipcp_data"];
        if (ipcp_data == 0) {
        	LOG_ERR("IPCP Descriptor does not contain ipcpData element");
        	return -1;
        }

	ipcp_desc.dif_name = ipcp_data.get("dif_name", string()).asString();
	ipcp_desc.ipcp_type = ipcp_data.get("type", string()).asString();
	ipcp_desc.dif_template_name = ipcp_data.get("template", string()).asString();
	difs = ipcp_data["registrations"];
	if (difs != 0) {
		for (unsigned int j = 0; j < difs.size(); j++) {
			ipcp_desc.difs_to_register_at.push_back(difs[j].asString());
		}
	}

        return 0;
}

int DIFTemplateManager::get_ipcp_config_from_desc(rinad::configs::ipcp_config_t & ipcp_config,
						  const std::string& system_name,
			     	     	     	  const std::string& desc_file)
{
	IPCPDescriptor ipcp_desc;

	if (parse_ipcp_descriptor(desc_file, ipcp_desc)) {
		return -1;
	}

	return __get_ipcp_config_from_desc(ipcp_config, system_name, ipcp_desc);
}

int DIFTemplateManager::__get_ipcp_config_from_desc(rinad::configs::ipcp_config_t & ipcp_config,
			      	      	      	  const std::string& system_name,
			      	      	      	  const IPCPDescriptor& ipcp_desc)
{
	rinad::DIFTemplate dift;
	std::stringstream ss;
	std::list<std::string>::const_iterator reg_it;
	std::list<rina::Neighbor>::const_iterator nei_it;
	rina::ApplicationProcessNamingInformation aux_name;
	rinad::configs::neighbor_config_t nei_conf;

	if (get_dif_template(ipcp_desc.dif_template_name, dift)) {
		return -1;
	}

	ss << system_name << "." << ipcp_desc.dif_name;
	ipcp_config.name.processName = ss.str();
	ipcp_config.name.processInstance = "1";
	ipcp_config.dif_to_assign.dif_type_ = ipcp_desc.ipcp_type;
	ipcp_config.dif_to_assign.dif_name_.processName = ipcp_desc.dif_name;

	if (ipcp_desc.ipcp_type == rina::NORMAL_IPC_PROCESS) {
		unsigned int address;

		ipcp_config.dif_to_assign.dif_configuration_.efcp_configuration_.data_transfer_constants_
			= dift.dataTransferConstants;

		rina::QoSCube * qosCube = 0;
		for (std::list<rina::QoSCube>::iterator qit = dift.qosCubes
				.begin(); qit != dift.qosCubes.end(); qit++)
		{
			qosCube = new rina::QoSCube(*qit);
			if (!qosCube)
			{
				LOG_ERR("Unable to allocate memory for the QoSCube object. Out of memory! %s",
					ipcp_desc.dif_name.c_str());
				return -1;
			}
			ipcp_config.dif_to_assign.dif_configuration_.efcp_configuration_.add_qos_cube(qosCube);
		}

		for (std::list<rinad::AddressPrefixConfiguration>::iterator ait =
				dift.addressPrefixes.begin();
				ait != dift.addressPrefixes.end(); ait++)
		{
			rina::AddressPrefixConfiguration prefix;
			prefix.address_prefix_ = ait->addressPrefix;
			prefix.organization_ = ait->organization;
			ipcp_config.dif_to_assign.dif_configuration_.nsm_configuration_.
				addressing_configuration_.address_prefixes_.push_back(prefix);
		}

		for (std::list<rinad::KnownIPCProcessAddress>::iterator kit =
				dift.knownIPCProcessAddresses.begin();
				kit != dift.knownIPCProcessAddresses.end(); kit++)
		{
			rina::StaticIPCProcessAddress static_address;
			static_address.ap_name_ = kit->name.processName;
			static_address.ap_instance_ = kit->name.processInstance;
			static_address.address_ = kit->address;
			ipcp_config.dif_to_assign.dif_configuration_.nsm_configuration_.
				addressing_configuration_.static_address_.push_back(static_address);
		}
		ipcp_config.dif_to_assign.dif_configuration_.nsm_configuration_.policy_set_ = dift.nsmConfiguration.policy_set_;

		bool found = dift.lookup_ipcp_address(ipcp_config.name, address);
		if (!found)
		{
			LOG_ERR("No address for IPC process %s ",
				ipcp_config.name.toString().c_str());
			return -1;
		}
		ipcp_config.dif_to_assign.dif_configuration_.rmt_configuration_ = dift.rmtConfiguration;
		ipcp_config.dif_to_assign.dif_configuration_.fa_configuration_ = dift.faConfiguration;
		ipcp_config.dif_to_assign.dif_configuration_.ra_configuration_ = dift.raConfiguration;
		ipcp_config.dif_to_assign.dif_configuration_.routing_configuration_ = dift.routingConfiguration;
		ipcp_config.dif_to_assign.dif_configuration_.sm_configuration_ = dift.secManConfiguration;
		ipcp_config.dif_to_assign.dif_configuration_.et_configuration_ = dift.etConfiguration;
		ipcp_config.dif_to_assign.dif_configuration_.set_address(address);
	}

	for (std::map<std::string, std::string>::const_iterator pit =
			dift.configParameters.begin();
			pit != dift.configParameters.end(); pit++) {
		ipcp_config.dif_to_assign.dif_configuration_.add_parameter(
				rina::PolicyParameter(pit->first, pit->second));
	}

	// Registrations to N-1 DIFs
	for (reg_it = ipcp_desc.difs_to_register_at.begin();
			reg_it != ipcp_desc.difs_to_register_at.end(); ++reg_it) {
		aux_name.processName = *reg_it;
		ipcp_config.difs_to_register.push_back(aux_name);
	}

	// Enrollments to neighbors in the N-DIF
	for (nei_it = ipcp_desc.neighbors.begin();
			nei_it != ipcp_desc.neighbors.end(); ++nei_it) {
		nei_conf.neighbor_name = nei_it->name_;
		nei_conf.under_dif = nei_it->supporting_dif_name_;
		nei_conf.dif.processName = ipcp_desc.dif_name;
		ipcp_config.neighbors.push_back(nei_conf);
	}

	return 0;
}
