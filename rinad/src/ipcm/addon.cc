
/*
 * Base class for an addon
 *
 *    Marc Sune <marc.sune (at) bisdn.de>
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

#include "addon.h"

#define RINA_PREFIX "ipcm.addon_factory"
#include <librina/logs.h>

//Addons
#include "addons/console.h"
#include "addons/mobility-manager.h"
#include "addons/scripting.h"
#include "addons/ma/agent.h"
//[+] Add more here...

namespace rinad {

//Static members
rina::ReadWriteLockable Addon::rwlock;
std::map<std::string, Addon*> Addon::addons;
std::list<Addon*> Addon::event_subscribers;

//Factory
void Addon::factory(rinad::RINAConfiguration& conf, const std::string& name){

	Addon* addon = NULL;

	try{
		//TODO this is a transitory solution. A proper auto-registering
		// to the factory would be the right way to go
		if(name == mad::ManagementAgent::NAME){
			addon = new mad::ManagementAgent(conf);
		}else if(name == IPCMConsole::NAME){
			addon = new IPCMConsole(conf.local.consoleSocket);
		}else if(name == ScriptingEngine::NAME){
			addon = new ScriptingEngine();
		}else if(name == MobilityManager::NAME){
			addon = new MobilityManager(conf);
		}else{
			//TODO add other types
			LOG_EMERG("Uknown addon name '%s'. Ignoring...", name.c_str());
			assert(0);
		}

		if(!addon){
			std::stringstream ss;
			ss << "FATAL: Unable to bootstrap addon name '"<< name <<"'.";
			rina::Exception(ss.str().c_str());
		}
	}catch(...){
		LOG_EMERG("Unable to bootstrap addon '%s'", name.c_str());
	}
}

void Addon::destroy_all(){

	std::map<std::string, Addon*>::iterator it;

	it = addons.begin();
	do{
		LOG_DBG("Destroying addon: %s(%p)", (it)->second->name.c_str(),
								it->second);
		delete it->second;
		addons.erase(it++);
	}while(it != addons.end());
}

Addon::Addon(const std::string _name):name(_name){

	rina::WriteScopedLock wlock(rwlock);

	//Check if it already exists
	if(addons.find(name) != addons.end()){
		std::stringstream ss;
		ss << "Addon name '"<< name <<"' already present. FATAL...";
		LOG_EMERG("%s", ss.str().c_str());
		throw rina::Exception(ss.str().c_str());
	}

	//Add to the addons list
	addons[name] = this;
}

// Distribute a librina event to the addons
void Addon::distribute_flow_event(rina::IPCEvent* event){

	unsigned int seqnum;
	std::list<Addon*>::const_iterator it;
	assert(event != NULL);

	if(!event)
		return;

	seqnum = event->sequenceNumber;

	rina::ReadScopedLock rlock(rwlock);

	for(it = event_subscribers.begin(); it != event_subscribers.end();
								 ++it){
		(*it)->process_librina_event(&event);
		if(!event)
			return;
	}

	if(event){
		LOG_WARN("Unprocessed event of type %s and sequence number %u.",
				rina::IPCEvent::eventTypeToString(event->eventType).c_str(),
				event->sequenceNumber);

		delete event;
	}
}

// Distribute an ipcm event to the addons
void Addon::distribute_ipcm_event(const IPCMEvent& event){

	std::map<std::string, Addon*>::iterator it;

	//Prevent creation/destruction of addons during loop
	rina::ReadScopedLock rlock(rwlock);

	for(it = addons.begin(); it != addons.end(); ++it){
		if(event.callee != it->second)
			it->second->process_ipcm_event(event);
	}

}

//Register to receive events
void Addon::subscribe(Addon* addon){

	rina::WriteScopedLock wlock(rwlock);

	try{
		event_subscribers.push_back(addon);
	}catch(...){
		LOG_EMERG("FATAL ERROR: unable to subscribe addon '%s' to receive IPC events; out of memory?",
							addon->name.c_str());
		assert(0);
		exit(EXIT_FAILURE);
	}
}

//Unregister from receiving events
void Addon::unsubscribe(Addon* addon){

	rina::WriteScopedLock wlock(rwlock);

	try{
		event_subscribers.remove(addon);
	}catch(...){
		LOG_WARN("Unable to unsubscribe addon '%s'.",
							addon->name.c_str());
		assert(0);
	}
}

} //namespace rinad
