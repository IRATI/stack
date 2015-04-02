#include "addon.h"


#define RINA_PREFIX "ipcm.addon_factory"
#include <librina/logs.h>

//Addons
#include "addons/console.h"
#include "addons/scripting.h"
#include "addons/ma/agent.h"
//[+] Add more here...

namespace rinad {

//Static members
rina::Lockable Addon::mutex;
std::list<Addon*> Addon::event_subscribers;

//Factory
Addon* Addon::factory(rinad::RINAConfiguration& conf, const std::string& name){

	Addon* addon = NULL;

	try{
		//TODO this is a transitory solution. A proper auto-registering
		// to the factory would be the right way to go
		if(name == "mad"){
			addon = new mad::ManagementAgent(std::string(""));
		}else if(name == "console"){
			addon = new IPCMConsole(conf.local.consolePort);
		}else if(name == "scripting"){
			addon = new ScriptingEngine();
		}else{
			//TODO add other types
			LOG_EMERG("Uknown addon name '%s'. Ignoring...", name.c_str());
			assert(0);
		}
	}catch(...){
		LOG_EMERG("Unable to bootstrap addon '%s'", name.c_str());
	}
	return addon;
}


// Distribute a librina event to the addons
void Addon::distribute_event(rina::IPCEvent* event){

	unsigned int seqnum;
	std::list<Addon*>::const_iterator it;
	assert(event != NULL);

	if(!event)
		return;

	seqnum = event->sequenceNumber;

	rina::ScopedLock lock(mutex);

	for(it = event_subscribers.begin(); it != event_subscribers.end();
								 ++it){
		(*it)->process_event(&event);
		if(!event)
			return;
	}

	if(event){
		LOG_WARN("Unprocessed event of type %s and sequence number %u. Destroying...",
				rina::IPCEvent::eventTypeToString(event->eventType).c_str(),
				event->sequenceNumber);

		assert(0);
		delete event;
	}
}

//Register to receive events
void Addon::subscribe(Addon* addon){

	rina::ScopedLock lock(mutex);
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

	rina::ScopedLock lock(mutex);

	try{
		event_subscribers.remove(addon);
	}catch(...){
		LOG_WARN("Unable to unsubscribe addon '%s'.",
							addon->name.c_str());
		assert(0);
	}
}




} //namespace rinad
