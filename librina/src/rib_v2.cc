/*
 * RIB API
 *
 *    Bernat Gastón <bernat.gaston@i2cat.net>
 *    Eduard Grasa <eduard.grasa@i2cat.net>
 *
 * This library is free software{} you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation{} either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY{} without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library{} if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301  USA
 */

#include <stdint.h>
#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>

#include <algorithm>
#include <map>
#define RINA_PREFIX "rib"
#include <librina/logs.h>
//FIXME iostream is only for debuging purposes
//#include <iostream>

#include "librina/rib_v2.h"
#include "librina/cdap.h"
#include "librina/cdap_v2.h"

namespace rina {
namespace rib {

//
// Internals Wrap over a RIBObj
//
// This class encapsulate the internal state required for a RIBObj, which is
// deliverately not exposed to the RIB library user
//
class RIBObjWrap {

	friend class RIB;

private:
	RIBObjWrap(RIBObj *object);
	~RIBObjWrap() throw ();

	bool add_child(RIBObjWrap *child);
	bool remove_child(const std::string& name);
	RIBObj* object_;
	RIBObjWrap* parent_;
	std::list<RIBObjWrap*> children_;

	//Mutex
	rina::Lockable mutex;
};

RIBObjWrap::RIBObjWrap(RIBObj *object){

	object_ = object;
}

RIBObjWrap::~RIBObjWrap() throw () {
	delete object_;
}

bool RIBObjWrap::add_child(RIBObjWrap *child) {
	mutex.lock();
	for (std::list<RIBObjWrap*>::iterator it = children_.begin();
			it != children_.end(); it++) {
		if ((*it)->object_->get_name().compare(
					child->object_->get_name()) == 0) {
			LOG_ERR("Object is already a child");
			mutex.unlock();
			return false;
		}
	}
	children_.push_back(child);
	child->parent_ = this;
	mutex.unlock();
	return true;
}

bool RIBObjWrap::remove_child(const std::string& name) {
	bool found = false;
	mutex.lock();
	for (std::list<RIBObjWrap*>::iterator it = children_.begin();
			it != children_.end(); it++) {
		if ((*it)->object_->get_name().compare(name) == 0) {
			children_.erase(it);
			found = true;
			break;
		}
	}
	mutex.unlock();
	if (!found)
		LOG_ERR("Unknown child object with object name: %s",
				name.c_str());
	return found;
}

//
// CLASS RIBSchemaObject
//
RIBSchemaObject::RIBSchemaObject(const std::string& class_name,
		const bool mandatory,
		const unsigned max_objs) {

	class_name_ = class_name;
	mandatory_ = mandatory;
	max_objs_ = max_objs;
	(void) parent_;
}

void RIBSchemaObject::addChild(RIBSchemaObject *object) {

	children_.push_back(object);
}

const std::string& RIBSchemaObject::get_class_name() const {

	return class_name_;
}

unsigned RIBSchemaObject::get_max_objs() const {

	return max_objs_;
}

//
// CLASS RIBSchema
//
RIBSchema::RIBSchema(const cdap_rib::vers_info_t *version, char separator) {

	version_ = version;
	separator_ = separator;
}
RIBSchema::~RIBSchema() {

	delete version_;
}

rib_schema_res RIBSchema::ribSchemaDefContRelation(
		const std::string& cont_class_name,
		const std::string& class_name, const bool mandatory,
		const unsigned max_objs) {

	RIBSchemaObject *object = new RIBSchemaObject(class_name, mandatory,
			max_objs);
	std::map<std::string, RIBSchemaObject*>::iterator parent_it =
		rib_schema_.find(cont_class_name);

	if (parent_it == rib_schema_.end())
		return RIB_SCHEMA_FORMAT_ERR;

	std::pair<std::map<std::string, RIBSchemaObject*>::iterator, bool> ret =
		rib_schema_.insert(
				std::pair<std::string, RIBSchemaObject*>(
					class_name, object));

	if (ret.second) {
		return RIB_SUCCESS;
	} else {
		return RIB_SCHEMA_FORMAT_ERR;
	}
}

bool RIBSchema::validateAddObject(const RIBObj* obj) {

	(void) obj;
	/*
	   RIBSchemaObject *schema_object = rib_schema_.find(obj->get_class());
	// CHECKS REGION //
	// Existance
	if (schema_object == 0)
	LOG_INFO();
	return false;
	// parent existance
	RIBSchemaObject *parent_schema_object = rib_schema_[obj->get_parent_class()];
	if (parent_schema_object == 0)
	return false;
	// maximum number of objects
	if (parent->get_children_size() >= schema_object->get_max_objs()) {
	return false;
	}
	 */
	return true;
}

char RIBSchema::get_separator() const {
	return separator_;
}

const cdap_rib::vers_info_t& RIBSchema::get_version() const {
	return *version_;
}


/// A simple RIB implementation, based on a hashtable of RIB objects
/// indexed by object name
class RIB {

public:
	RIB(const RIBSchema *schema,
				cdap::CDAPProviderInterface *cdap_provider);
	~RIB();

	/// Given an objectname of the form "substring\0substring\0...substring" locate
	/// the RIBObj that corresponds to it
	/// @param objectName
	/// @return
	RIBObj* getRIBObj(const std::string& clas,
			const std::string& name, bool check=false);
	RIBObj* getRIBObj(const std::string& clas, long instance,
			bool check=false);
	RIBObj* removeRIBObj(const std::string& name);
	RIBObj* removeRIBObj(long instance);
	char get_separator() const;
	void addRIBObj(RIBObj* rib_object);
	std::string get_parent_name(const std::string child_name) const;
	const cdap_rib::vers_info_t& get_version() const;

	//
	// Incoming requests to the local RIB
	//
	void create_request(const cdap_rib::con_handle_t &con,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::filt_info_t &filt,
			int message_id);
	void delete_request(const cdap_rib::con_handle_t &con,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::filt_info_t &filt,
			int message_id);
	void read_request(const cdap_rib::con_handle_t &con,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::filt_info_t &filt,
			int message_id);
	void cancel_read_request(const cdap_rib::con_handle_t &con,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::filt_info_t &filt,
			int message_id);
	void write_request(const cdap_rib::con_handle_t &con,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::filt_info_t &filt,
			int message_id);
	void start_request(const cdap_rib::con_handle_t &con,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::filt_info_t &filt,
			int message_id);
	void stop_request(const cdap_rib::con_handle_t &con,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::filt_info_t &filt,
			int message_id);
private:
	std::map<std::string, RIBObjWrap*> obj_name_map;
	std::map<long, RIBObjWrap*> obj_inst_map;
	const RIBSchema *schema;

	RIBObjWrap* getInternalObj(const std::string& name);
	RIBObjWrap* getInternalObj(long instance);

	//CDAP Provider
	cdap::CDAPProviderInterface *cdap_provider;

	//rwlock
	ReadWriteLockable rwlock;
};

RIB::RIB(const RIBSchema *schema_,
				cdap::CDAPProviderInterface *cdap_provider_){

	schema = schema_;
	cdap_provider = cdap_provider_;
}

RIB::~RIB() {

	//Mutual exclusion
	WriteScopedLock wlock(rwlock);

	for (std::map<std::string, RIBObjWrap*>::iterator it = obj_name_map
			.begin(); it != obj_name_map.end(); ++it) {
		LOG_INFO("Object %s removed from the RIB",
				it->second->object_->get_name().c_str());
		delete it->second;
	}
	obj_name_map.clear();
	obj_inst_map.clear();
}

void RIB::create_request(const cdap_rib::con_handle_t &con,
		const cdap_rib::obj_info_t &obj,
		const cdap_rib::filt_info_t &filt,
		int message_id) {

	(void) filt;
	// FIXME add res and flags
	cdap_rib::flags_t flags;
	flags.flags_ = cdap_rib::flags_t::NONE_FLAGS;

	//Reply object set to empty
	cdap_rib::obj_info_t obj_reply;
	obj_reply.name_ = obj.name_;
	obj_reply.class_ = obj.class_;
	obj_reply.inst_ = obj.inst_;
	obj_reply.value_.size_ = 0;
	obj_reply.value_.message_ = NULL;

	cdap_rib::res_info_t* res;
	RIBObj* rib_obj = getRIBObj(obj.class_, obj.name_,
			true);
	if (rib_obj == NULL) {
		std::string parent_name = get_parent_name(obj.name_);
		rib_obj = getRIBObj("", parent_name, false);
	}
	if (rib_obj) {
		//Call the application
		res = rib_obj->create(
				obj.name_, obj.class_, obj.value_,
				obj_reply.value_);
	} else {
		res = new cdap_rib::res_info_t;
		res->result_ = -1;
	}
	try {
		cdap_provider->send_create_result(con.port_,
				obj_reply,
				flags, *res,
				message_id);
	} catch (Exception &e) {
		LOG_ERR("Unable to send the response");
	}
	delete res;
}

void RIB::delete_request(const cdap_rib::con_handle_t &con,
		const cdap_rib::obj_info_t &obj,
		const cdap_rib::filt_info_t &filt,
		int message_id) {

	(void) filt;
	// FIXME add res and flags
	cdap_rib::flags_t flags;

	RIBObj* rib_obj = getRIBObj(obj.class_, obj.name_,
			true);
	if (rib_obj) {
		cdap_rib::res_info_t* res = rib_obj->delete_(obj.name_);
		try {
			cdap_provider->send_delete_result(con.port_, obj,
					flags, *res,
					message_id);
		} catch (Exception &e) {
			LOG_ERR("Unable to send the response");
		}
		delete res;
	}
}
void RIB::read_request(const cdap_rib::con_handle_t &con,
		const cdap_rib::obj_info_t &obj,
		const cdap_rib::filt_info_t &filt,
		int message_id) {

	(void) filt;
	// FIXME add res and flags
	cdap_rib::flags_t flags;
	flags.flags_ = cdap_rib::flags_t::NONE_FLAGS;

	//Reply object set to empty
	cdap_rib::obj_info_t obj_reply;
	obj_reply.name_ = obj.name_;
	obj_reply.class_ = obj.class_;
	obj_reply.inst_ = obj.inst_;
	obj_reply.value_.size_ = 0;
	obj_reply.value_.message_ = NULL;

	cdap_rib::res_info_t* res;
	RIBObj* ribObj = getRIBObj(obj.class_, obj.name_, true);
	if (ribObj) {
		res = ribObj->read(
				obj.name_, obj_reply.value_);
	} else {
		res = new cdap_rib::res_info_t;
		res->result_ = -1;
	}
	try {
		cdap_provider->send_read_result(con.port_,
				obj_reply,
				flags, *res,
				message_id);
	} catch (Exception &e) {
		LOG_ERR("Unable to send the response");
	}
	delete res;
}
void RIB::cancel_read_request(
		const cdap_rib::con_handle_t &con,
		const cdap_rib::obj_info_t &obj,
		const cdap_rib::filt_info_t &filt, int message_id) {

	(void) filt;

	// FIXME add res and flags
	cdap_rib::flags_t flags;
	cdap_rib::res_info_t* res;

	RIBObj* ribObj = getRIBObj(obj.class_, obj.name_, true);
	if (ribObj) {
		res = ribObj->cancelRead(obj.name_);
	} else {
		res = new cdap_rib::res_info_t;
		res->result_ = -1;
	}

	try {
		cdap_provider->send_cancel_read_result(
				con.port_, flags, *res, message_id);
	} catch (Exception &e) {
		LOG_ERR("Unable to send the response");
	}
	delete res;
}
void RIB::write_request(const cdap_rib::con_handle_t &con,
		const cdap_rib::obj_info_t &obj,
		const cdap_rib::filt_info_t &filt,
		int message_id) {

	(void) filt;
	// FIXME add res and flags
	cdap_rib::flags_t flags;

	//Reply object set to empty
	cdap_rib::obj_info_t obj_reply;
	obj_reply.name_ = obj.name_;
	obj_reply.class_ = obj.class_;
	obj_reply.inst_ = obj.inst_;
	obj_reply.value_.size_ = 0;
	obj_reply.value_.message_ = NULL;

	cdap_rib::res_info_t* res;

	RIBObj* ribObj = getRIBObj(obj.class_, obj.name_, true);
	if (ribObj) {
		res = ribObj->write(
				obj.name_, obj.value_, obj_reply.value_);
	} else {
		res = new cdap_rib::res_info_t;
		res->result_ = -1;
	}
	try {
		cdap_provider->send_write_result(con.port_, flags,
				*res,
				message_id);
	} catch (Exception &e) {
		LOG_ERR("Unable to send the response");
	}
	delete res;
}
void RIB::start_request(const cdap_rib::con_handle_t &con,
		const cdap_rib::obj_info_t &obj,
		const cdap_rib::filt_info_t &filt,
		int message_id) {

	(void) filt;
	// FIXME add res and flags
	cdap_rib::flags_t flags;

	//Reply object set to empty
	cdap_rib::obj_info_t obj_reply;
	obj_reply.value_.size_ = 0;
	obj_reply.name_ = obj.name_;
	obj_reply.class_ = obj.class_;
	obj_reply.inst_ = obj.inst_;
	obj_reply.value_.size_ = 0;
	obj_reply.value_.message_ = NULL;

	cdap_rib::res_info_t* res;

	RIBObj* ribObj = getRIBObj(obj.class_, obj.name_, true);
	if (ribObj) {
		res = ribObj->start(
				obj.name_, obj.value_, obj_reply.value_);
	} else {
		res = new cdap_rib::res_info_t;
		res->result_ = -1;
	}
	try {
		cdap_provider->send_start_result(con.port_, obj,
				flags, *res,
				message_id);
	} catch (Exception &e) {
		LOG_ERR("Unable to send the response");
	}
	delete res;
}
void RIB::stop_request(const cdap_rib::con_handle_t &con,
		const cdap_rib::obj_info_t &obj,
		const cdap_rib::filt_info_t &filt,
		int message_id) {

	(void) filt;
	// FIXME add res and flags
	cdap_rib::flags_t flags;

	//Reply object set to empty
	cdap_rib::obj_info_t obj_reply;
	obj_reply.name_ = obj.name_;
	obj_reply.class_ = obj.class_;
	obj_reply.inst_ = obj.inst_;
	obj_reply.value_.size_ = 0;
	obj_reply.value_.message_ = NULL;

	cdap_rib::res_info_t* res;

	RIBObj* ribObj = getRIBObj(obj.class_, obj.name_, true);
	if (ribObj) {
		res = ribObj->stop(
				obj.name_, obj.value_, obj_reply.value_);
	} else {
		res = new cdap_rib::res_info_t;
		res->result_ = -1;
	}
	try {
		cdap_provider->send_stop_result(con.port_, flags,
				*res,
				message_id);
	} catch (Exception &e) {
		LOG_ERR("Unable to send the response");
	}
	delete res;
}


RIBObj* RIB::getRIBObj(const std::string& clas,
		const std::string& name, bool check) {


	RIBObjWrap* rib_object = getInternalObj(name);

	if (!rib_object)
		return NULL;

	if (check && rib_object->object_->get_class().compare(clas) != 0) {
		LOG_ERR("RIB object class does not match the user specified one");
		return NULL;
	}

	return rib_object->object_;
}

RIBObj* RIB::getRIBObj(const std::string& clas, long instance,
		bool check) {

	RIBObjWrap* rib_object = getInternalObj(instance);

	if (!rib_object)
		return NULL;

	if (check && rib_object->object_->get_class().compare(clas) != 0) {
		LOG_ERR("RIB object class does not match the user specified one");
		return NULL;
	}

	return rib_object->object_;
}

void RIB::addRIBObj(RIBObj* rib_object) {

	RIBObjWrap *parent = NULL;
	RIBObjWrap *obj = NULL;

	// Check if the parent exists
	std::string parent_name = get_parent_name(rib_object->get_name());
	if (!parent_name.empty()) {
		parent = getInternalObj(parent_name);
		if (!parent) {
			std::stringstream ss;
			ss << "Exception in object " << rib_object->get_name()
				<< ". Parent name (" << parent_name
				<< ") is not in the RIB" << std::endl;
			throw Exception(ss.str().c_str());
		}
	}
	// TODO: add schema validation
	//  if (rib_schema_->validateAddObject(rib_object, parent))
	//  {

	obj = getInternalObj(rib_object->get_name());
	if (obj) {
		std::stringstream ss;
		ss << "Object with the same name (" << obj->object_->get_name()
			<< ") already exists in the RIB" << std::endl;
		throw Exception(ss.str().c_str());
	}
	obj = getInternalObj(rib_object->get_instance());
	if (obj) {
		std::stringstream ss;
		ss << "Object with the same instance ("
			<< rib_object->get_instance() << ") already exists "
			"in the RIB"
			<< std::endl;
		throw Exception(ss.str().c_str());
	}
	RIBObjWrap *int_object = new RIBObjWrap(rib_object);
	if (parent) {
		if (!parent->add_child(int_object)) {
			std::stringstream ss;
			ss << "Can not add object '"
				<< int_object->object_->get_name()
				<< "' as a child of object '"
				<< parent->object_->get_name() << std::endl;
			throw Exception(ss.str().c_str());
		}
		int_object->parent_ = parent;
	}
	LOG_DBG("Object %s added to the RIB",
			int_object->object_->get_name().c_str());

	WriteScopedLock wlock(rwlock);

	obj_name_map[int_object->object_->get_name()] = int_object;
	obj_inst_map[int_object->object_->get_instance()] = int_object;
}

RIBObj* RIB::removeRIBObj(const std::string& name) {
	RIBObjWrap* rib_object = getInternalObj(name);

	if (!rib_object) {
		LOG_ERR("RIB object %s is not in the RIB", name.c_str());
		return NULL;
	}

	RIBObjWrap* parent = rib_object->parent_;

	if (parent) {
		if (!parent->remove_child(
					rib_object->object_->get_name())) {
			std::stringstream ss;
			ss << "Can not remove object '"
				<< rib_object->object_->get_name()
				<< "' as a child of object '"
				<< parent->object_->get_name()
				<< std::endl;
			throw Exception(ss.str().c_str());
		}
	}

	WriteScopedLock wlock(rwlock);

	obj_name_map.erase(rib_object->object_->get_name());
	obj_inst_map.erase(rib_object->object_->get_instance());

	LOG_DBG("Object %s removed from the RIB",
				rib_object->object_->get_name().c_str());
	return rib_object->object_;
}

RIBObj * RIB::removeRIBObj(long instance) {
	RIBObjWrap* rib_object = getInternalObj(instance);

	if (!rib_object){
		LOG_ERR("RIB object %d is not in the RIB", instance);
		return NULL;
	}

	RIBObjWrap* parent = rib_object->parent_;
	parent->remove_child(rib_object->object_->get_name());

	WriteScopedLock wlock(rwlock);

	obj_name_map.erase(rib_object->object_->get_name());
	obj_inst_map.erase(instance);

	return rib_object->object_;
}

char RIB::get_separator() const {

	return schema->get_separator();
}

std::string RIB::get_parent_name(const std::string child_name) const {

	size_t last_separator = child_name.find_last_of(schema->separator_,
			std::string::npos);
	if (last_separator == std::string::npos)
		return "";

	return child_name.substr(0, last_separator);

}

const cdap_rib::vers_info_t& RIB::get_version() const {
	return schema->get_version();
}

RIBObjWrap* RIB::getInternalObj(const std::string& name) {
	std::string norm_name = name;
	norm_name.erase(std::remove_if(norm_name.begin(), norm_name.end(),
				::isspace),
			norm_name.end());

	std::map<std::string, RIBObjWrap*>::iterator it;

	it = obj_name_map.find(norm_name);

	if (it == obj_name_map.end())
		return NULL;

	return it->second;
}

RIBObjWrap* RIB::getInternalObj(long instance) {
	std::map<long, RIBObjWrap*>::iterator it;

	it = obj_inst_map.find(instance);

	if (it == obj_inst_map.end())
		return NULL;

	return it->second;
}

///
/// RIBDaemon main class
///
class RIBDaemon : public cdap::CDAPCallbackInterface {

public:
	RIBDaemon(cacep::AppConHandlerInterface *app_con_callback,
			RIBOpsRespHandlers* remote_handlers,
			cdap_rib::cdap_params params);
	~RIBDaemon();

	//Public API

	///
	/// Register a RIB
	///
	void registerRIB(RIB* rib);

	///
	/// List registered RIB versions
	///
	std::list<uint64_t> listVersions(void);

	///
	/// Retrieve a pointer to the RIB
	///
	/// @param version RIB version
	/// @param Application Entity Name
	///
	/// @ret A pointer to the RIB object or NULL. The application needs to
	/// take care to safely access it.
	///
	RIB* get(const uint64_t version, const std::string& ae_name);

	/// Unregister a RIB
	///
	/// Unregisters a RIB from the library. This method does NOT
	/// destroy the RIB instance.
	///
	/// @ret On success, it returns the pointer to the RIB instance
	///
	RIB* unregisterRIB(RIB* inst);


protected:
	//
	// CDAP provider callbacks
	//

	//Remote
	void remote_open_connection_result(const cdap_rib::con_handle_t &con,
			const cdap_rib::result_info &res);
	void remote_close_connection_result(const cdap_rib::con_handle_t &con,
			const cdap_rib::result_info &res);
	void remote_create_result(const cdap_rib::con_handle_t &con,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::res_info_t &res);
	void remote_delete_result(const cdap_rib::con_handle_t &con,
			const cdap_rib::res_info_t &res);
	void remote_read_result(const cdap_rib::con_handle_t &con,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::res_info_t &res);
	void remote_cancel_read_result(const cdap_rib::con_handle_t &con,
			const cdap_rib::res_info_t &res);
	void remote_write_result(const cdap_rib::con_handle_t &con,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::res_info_t &res);
	void remote_start_result(const cdap_rib::con_handle_t &con,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::res_info_t &res);
	void remote_stop_result(const cdap_rib::con_handle_t &con,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::res_info_t &res);


	//Local
	void open_connection(const cdap_rib::con_handle_t &con,
			const cdap_rib::flags_t &flags, int message_id);
	void close_connection(const cdap_rib::con_handle_t &con,
			const cdap_rib::flags_t &flags, int message_id);

	void create_request(const cdap_rib::con_handle_t &con,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::filt_info_t &filt,
			int message_id);
	void delete_request(const cdap_rib::con_handle_t &con,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::filt_info_t &filt,
			int message_id);
	void read_request(const cdap_rib::con_handle_t &con,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::filt_info_t &filt,
			int message_id);
	void cancel_read_request(const cdap_rib::con_handle_t &con,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::filt_info_t &filt,
			int message_id);
	void write_request(const cdap_rib::con_handle_t &con,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::filt_info_t &filt,
			int message_id);
	void start_request(const cdap_rib::con_handle_t &con,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::filt_info_t &filt,
			int message_id);
	void stop_request(const cdap_rib::con_handle_t &con,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::filt_info_t &filt,
			int message_id);


	//
	//Incoming connections (proxy only)
	//
	void store_connection(const cdap_rib::con_handle_t& con);
	void remove_connection(const cdap_rib::con_handle_t& con);

private:

	//Friendship
	friend class RIBDaemonProxy;

	/**
	* @internal Get the RIB by port_id.
	* TODO: this should be deprecated in favour of putting some state
	* in the (opaque pointer, sort of cookie)
	*/
	RIB* getByPortId(const int port_id);

	cacep::AppConHandlerInterface *app_con_callback_;
	cdap::CDAPProviderInterface *cdap_provider;
	RIBOpsRespHandlers* remote_handlers;

	//RIB instances (all)
	std::list<RIB*> ribs;

	//Port id <-> RIB
	std::map<int, RIB*> port_id_rib_map;

	//TODO add per version and per AE

	/**
	 * read/write lock
	 */
	ReadWriteLockable rwlock;
};


//
// Constructors and destructors
//

RIBDaemon::RIBDaemon(cacep::AppConHandlerInterface *app_con_callback,
		RIBOpsRespHandlers* remote_handlers_,
		cdap_rib::cdap_params params) {

	app_con_callback_ = app_con_callback;

	//Initialize the parameters
	//add cdap parameters
	cdap::init(this, params.is_IPCP_);
	cdap_provider = cdap::getProvider();
	remote_handlers = remote_handlers_;
}

RIBDaemon::~RIBDaemon() {
	delete app_con_callback_;
}

//
/// Register a RIB
///
void RIBDaemon::registerRIB(RIB* rib){

	//Mutual exclusion
	WriteScopedLock wlock(rwlock);

	//Check whether it exists already
	if (std::find(ribs.begin(), ribs.end(), rib) != ribs.end()) {
		LOG_ERR("Attempting to register an existing RIB: %p", rib);
		throw eRIBVersionExists();
	}

	ribs.push_back(rib);
}

///
/// List registered RIB versions
///
std::list<uint64_t> RIBDaemon::listVersions(void){
	std::list<uint64_t> vers;
	uint64_t ver;

	//Mutual exclusion
	ReadScopedLock rlock(rwlock);

	//Copy keys
	for(std::list<RIB*>::const_iterator it = ribs.begin();
			it != ribs.end(); ++it){
		cdap_rib::vers_info_t version = (*it)->get_version();
		ver = version.version_;
		if (std::find(vers.begin(), vers.end(), ver) ==
								vers.end())
			vers.push_back(ver);
	}

	return vers;
}

RIB* RIBDaemon::get(const uint64_t ver, const std::string& ae_name){

	uint64_t it_ver;
	//TODO filter by ae_name
	(void)ae_name;

	//Mutual exclusion
	ReadScopedLock rlock(rwlock);

	for(std::list<RIB*>::const_iterator it = ribs.begin();
							it != ribs.end();
							++it){
		it_ver = (*it)->get_version().version_;
		if(it_ver == ver)
			return *it;
	}

	return NULL;
}


RIB* RIBDaemon::getByPortId(const int port_id){

	//Mutual exclusion
	ReadScopedLock rlock(rwlock);

	if(port_id_rib_map.find(port_id) == port_id_rib_map.end())
		return NULL;
	return port_id_rib_map[port_id];
}

void RIBDaemon::store_connection(const cdap_rib::con_handle_t& con){

	const uint64_t ver = con.version_.version_;
	const std::string& ae = con.dest_.ae_name_;
	const int port_id = con.port_;

	//FIXME: what if the rib is destroyed...
	RIB* rib = get(ver, ae);

	if(!rib){
		LOG_ERR("No RIB version %" PRIu64 " registered for AE %s!",
								ver,
								ae.c_str());
		throw eRIBVersionDoesNotExist();
	}

	//Mutual exclusion
	WriteScopedLock wlock(rwlock);

	//Store relation RIB <-> port_id
	if(port_id_rib_map.find(port_id) != port_id_rib_map.end()){
		LOG_ERR("Overwritting previous connection for RIB version: %" PRIu64 ", AE: %s and port id: %d!",
								ver,
								ae.c_str(),
								port_id);
		assert(0);
	}

	port_id_rib_map[port_id] = rib;

	LOG_INFO("Bound port_id: %d CDAP connection to RIB version %" PRIu64 " (AE %s)",
								port_id,
								ver,
								ae.c_str());
}

void RIBDaemon::remove_connection(const cdap_rib::con_handle_t& con){

	const uint64_t ver = con.version_.version_;
	const std::string& ae = con.dest_.ae_name_;
	const int port_id = con.port_;


	//FIXME: what if the rib is destroyed...
	RIB* rib = getByPortId(port_id);

	if(!rib){
		LOG_ERR("Could not remove connection for port id: %d!",
								port_id);
		assert(0);
		return;
	}

	//TODO: assert version

	//Mutual exclusion
	WriteScopedLock wlock(rwlock);

	//Remove from the map
	port_id_rib_map.erase(port_id);

	LOG_INFO("CDAP connection on port id: %d unbound from RIB version %" PRIu64 " (AE %s)",
								port_id,
								ver,
								ae.c_str());
}
///
/// Unregister RIB
///
RIB* RIBDaemon::unregisterRIB(RIB* inst){

	//Mutual exclusion
	WriteScopedLock wlock(rwlock);

	std::list<RIB*>::iterator it = std::find(ribs.begin(),
							ribs.end(), inst);

	//Check whether it exists already
	if (it == ribs.end()) {
		LOG_ERR("Attempting to unregister RIB: %p", inst);
		throw eRIBVersionDoesNotExist();
	}

	ribs.erase(it);

	return inst;
}

//
// Connection callbacks
//

void RIBDaemon::remote_open_connection_result(const cdap_rib::con_handle_t &con,
		const cdap_rib::res_info_t &res) {

	// FIXME remove message_id

	app_con_callback_->connectResult(res, con);
}

void RIBDaemon::open_connection(const cdap_rib::con_handle_t &con,
		const cdap_rib::flags_t &flags,
		int message_id) {

	// FIXME add result
	cdap_rib::result_info res;
	(void) res;
	(void) flags;
	app_con_callback_->connect(message_id, con);

	//The connect was successful store
	store_connection(con);

	cdap_provider->send_open_connection_result(con, res, message_id);
}

void RIBDaemon::remote_close_connection_result(const cdap_rib::con_handle_t &con,
		const cdap_rib::result_info &res) {

	app_con_callback_->releaseResult(res, con);
	remove_connection(con);
}

void RIBDaemon::close_connection(const cdap_rib::con_handle_t &con,
		const cdap_rib::flags_t &flags,
		int message_id) {

	// FIXME add result
	cdap_rib::result_info res;
	(void) res;
	app_con_callback_->release(message_id, con);
	cdap_provider->send_close_connection_result(con.port_, flags, res,
			message_id);
}


//
// Callbacks from events coming from the CDAP provider
//

//Connection events

void RIBDaemon::remote_create_result(const cdap_rib::con_handle_t &con,
		const cdap_rib::obj_info_t &obj,
		const cdap_rib::res_info_t &res) {
	//TODO this is not safe if RIB instances can be deleted
	RIB* rib = getByPortId(con.port_);

	if(!rib)
		return;

	try {
		remote_handlers->remoteCreateResult(con, obj, res);
	} catch (Exception &e) {
		LOG_ERR("Unable to process the response");
	}
}
void RIBDaemon::remote_delete_result(const cdap_rib::con_handle_t &con,
		const cdap_rib::res_info_t &res) {
	//TODO this is not safe if RIB instances can be deleted
	RIB* rib = getByPortId(con.port_);

	if(!rib)
		return;

	try {
		remote_handlers->remoteDeleteResult(con, res);
	} catch (Exception &e) {
		LOG_ERR("Unable to process delete result");
	}
}
void RIBDaemon::remote_read_result(const cdap_rib::con_handle_t &con,
		const cdap_rib::obj_info_t &obj,
		const cdap_rib::res_info_t &res) {
	//TODO this is not safe if RIB instances can be deleted
	RIB* rib = getByPortId(con.port_);

	if(!rib)
		return;

	try {
		remote_handlers->remoteReadResult(con, obj, res);
	} catch (Exception &e) {
		LOG_ERR("Unable to process read result");
	}
}
void RIBDaemon::remote_cancel_read_result(
		const cdap_rib::con_handle_t &con,
		const cdap_rib::res_info_t &res) {

	//TODO this is not safe if RIB instances can be deleted
	RIB* rib = getByPortId(con.port_);

	if(!rib)
		return;

	try {
		remote_handlers->remoteCancelReadResult(con, res);
	} catch (Exception &e) {
		LOG_ERR("Unable to process cancel read result");
	}
}
void RIBDaemon::remote_write_result(const cdap_rib::con_handle_t &con,
		const cdap_rib::obj_info_t &obj,
		const cdap_rib::res_info_t &res) {

	//TODO this is not safe if RIB instances can be deleted
	RIB* rib = getByPortId(con.port_);

	if(!rib)
		return;

	try {
		remote_handlers->remoteWriteResult(con, obj, res);
	} catch (Exception &e) {
		LOG_ERR("Unable to process write result");
	}
}
void RIBDaemon::remote_start_result(const cdap_rib::con_handle_t &con,
		const cdap_rib::obj_info_t &obj,
		const cdap_rib::res_info_t &res) {

	//TODO this is not safe if RIB instances can be deleted
	RIB* rib = getByPortId(con.port_);

	if(!rib)
		return;

	try {
		remote_handlers->remoteStartResult(con, obj, res);
	} catch (Exception &e) {
		LOG_ERR("Unable to process start result");
	}
}
void RIBDaemon::remote_stop_result(const cdap_rib::con_handle_t &con,
		const cdap_rib::obj_info_t &obj,
		const cdap_rib::res_info_t &res) {

	//TODO this is not safe if RIB instances can be deleted
	RIB* rib = getByPortId(con.port_);

	if(!rib)
		return;

	try {
		remote_handlers->remoteStopResult(con, obj, res);
	} catch (Exception &e) {
		LOG_ERR("Unable to process stop result");
	}
}

//CDAP object operations
void RIBDaemon::create_request(const cdap_rib::con_handle_t &con,
		const cdap_rib::obj_info_t &obj,
		const cdap_rib::filt_info_t &filt,
		int message_id) {

	//TODO this is not safe if RIB instances can be deleted
	RIB* rib = getByPortId(con.port_);

	if(!rib){
		cdap_rib::res_info_t res;
		cdap_rib::flags_t flags;
		res.result_ = -1;
		try {
			cdap_provider->send_create_result(con.port_,
					obj,
					flags, res,
					message_id);
		} catch (Exception &e) {
			LOG_ERR("Unable to send the response");
		}
		return;
	}

	//Invoke
	rib->create_request(con, obj, filt, message_id);
}

void RIBDaemon::delete_request(const cdap_rib::con_handle_t &con,
		const cdap_rib::obj_info_t &obj,
		const cdap_rib::filt_info_t &filt,
		int message_id) {

	//TODO this is not safe if RIB instances can be deleted
	RIB* rib = getByPortId(con.port_);

	if(!rib){
		cdap_rib::res_info_t res;
		cdap_rib::flags_t flags;
		res.result_ = -1;
		try {
			cdap_provider->send_delete_result(con.port_, obj,
					flags, res,
					message_id);
		} catch (Exception &e) {
			LOG_ERR("Unable to send the response");
		}
		return;
	}

	//Invoke
	rib->delete_request(con, obj, filt, message_id);
}

void RIBDaemon::read_request(const cdap_rib::con_handle_t &con,
		const cdap_rib::obj_info_t &obj,
		const cdap_rib::filt_info_t &filt,
		int message_id) {

	//TODO this is not safe if RIB instances can be deleted
	RIB* rib = getByPortId(con.port_);

	if(!rib){
		cdap_rib::res_info_t res;
		cdap_rib::flags_t flags;
		res.result_ = -1;
		try {
			cdap_provider->send_read_result(con.port_, obj,
					flags, res,
					message_id);
		} catch (Exception &e) {
			LOG_ERR("Unable to send the response");
		}
		return;
	}

	//Invoke
	rib->read_request(con, obj, filt, message_id);
}
void RIBDaemon::cancel_read_request(
		const cdap_rib::con_handle_t &con,
		const cdap_rib::obj_info_t &obj,
		const cdap_rib::filt_info_t &filt, int message_id) {

	//TODO this is not safe if RIB instances can be deleted
	RIB* rib = getByPortId(con.port_);

	if(!rib){
		cdap_rib::res_info_t res;
		cdap_rib::flags_t flags;
		res.result_ = -1;
		try {
			cdap_provider->send_cancel_read_result(
					con.port_,
					flags, res,
					message_id);
		} catch (Exception &e) {
			LOG_ERR("Unable to send the response");
		}
		return;
	}

	//Invoke
	rib->cancel_read_request(con, obj, filt, message_id);
}
void RIBDaemon::write_request(const cdap_rib::con_handle_t &con,
		const cdap_rib::obj_info_t &obj,
		const cdap_rib::filt_info_t &filt,
		int message_id) {

	//TODO this is not safe if RIB instances can be deleted
	RIB* rib = getByPortId(con.port_);

	if(!rib){
		cdap_rib::res_info_t res;
		cdap_rib::flags_t flags;
		res.result_ = -1;
		try {
			cdap_provider->send_write_result(
					con.port_,
					flags, res,
					message_id);
		} catch (Exception &e) {
			LOG_ERR("Unable to send the response");
		}
		return;
	}

	//Invoke
	rib->write_request(con, obj, filt, message_id);
}
void RIBDaemon::start_request(const cdap_rib::con_handle_t &con,
		const cdap_rib::obj_info_t &obj,
		const cdap_rib::filt_info_t &filt,
		int message_id) {

	//TODO this is not safe if RIB instances can be deleted
	RIB* rib = getByPortId(con.port_);

	if(!rib){
		cdap_rib::res_info_t res;
		cdap_rib::flags_t flags;
		res.result_ = -1;
		try {
			cdap_provider->send_start_result(
					con.port_, obj,
					flags, res,
					message_id);
		} catch (Exception &e) {
			LOG_ERR("Unable to send the response");
		}
		return;
	}

	//Invoke
	rib->start_request(con, obj, filt, message_id);
}
void RIBDaemon::stop_request(const cdap_rib::con_handle_t &con,
		const cdap_rib::obj_info_t &obj,
		const cdap_rib::filt_info_t &filt,
		int message_id) {

	//TODO this is not safe if RIB instances can be deleted
	RIB* rib = getByPortId(con.port_);

	if(!rib){
		cdap_rib::res_info_t res;
		cdap_rib::flags_t flags;
		res.result_ = -1;
		try {
			cdap_provider->send_stop_result(
					con.port_,
					flags, res,
					message_id);
		} catch (Exception &e) {
			LOG_ERR("Unable to send the response");
		}
		return;
	}

	//Invoke
	rib->stop_request(con, obj, filt, message_id);

}

//
// Encoder AbstractEncoder and base class
//

AbstractEncoder::~AbstractEncoder() {
}

bool AbstractEncoder::operator=(const AbstractEncoder &other) const {

	if (get_type() == other.get_type())
		return true;
	else
		return false;
}

bool AbstractEncoder::operator!=(const AbstractEncoder &other) const {

	if (get_type() != other.get_type())
		return true;
	else
		return false;
}

//RIBObj
std::string RIBObj::get_displayable_value() {

	return "-";
}

cdap_rib::res_info_t* RIBObj::create(
		const std::string& name, const std::string clas,
		const cdap_rib::SerializedObject &obj_req,
		cdap_rib::SerializedObject &obj_reply) {

	(void) name;
	(void) clas;
	(void) obj_req;
	(void) obj_reply;
	cdap_rib::res_info_t* res= new cdap_rib::res_info_t;
	operation_not_supported();
	// FIXME: change for real opcode
	res->result_ = -2;
	return res;
}

cdap_rib::res_info_t* RIBObj::delete_(const std::string& name) {

	(void) name;
	operation_not_supported();
	cdap_rib::res_info_t* res= new cdap_rib::res_info_t;
	// FIXME: change for real opcode
	res->result_ = -2;
	return res;
}

// FIXME remove name, it is not needed
cdap_rib::res_info_t* RIBObj::read(
		const std::string& name,
		cdap_rib::SerializedObject &obj_reply) {

	(void) name;
	(void) obj_reply;
	operation_not_supported();
	cdap_rib::res_info_t* res= new cdap_rib::res_info_t;
	// FIXME: change for real opcode
	res->result_ = -2;
	return res;
}

cdap_rib::res_info_t* RIBObj::cancelRead(const std::string& name) {

	(void) name;
	operation_not_supported();
	cdap_rib::res_info_t* res= new cdap_rib::res_info_t;
	// FIXME: change for real opcode
	res->result_ = -2;
	return res;
}

cdap_rib::res_info_t* RIBObj::write(
		const std::string& name,
		const cdap_rib::SerializedObject &obj_req,
		cdap_rib::SerializedObject &obj_reply) {

	(void) name;
	(void) obj_req;
	(void) obj_reply;
	operation_not_supported();
	cdap_rib::res_info_t* res= new cdap_rib::res_info_t;
	// FIXME: change for real opcode
	res->result_ = -2;
	return res;
}

cdap_rib::res_info_t* RIBObj::start(
		const std::string& name,
		const cdap_rib::SerializedObject &obj_req,
		cdap_rib::SerializedObject &obj_reply) {

	(void) name;
	(void) obj_req;
	(void) obj_reply;
	operation_not_supported();
	cdap_rib::res_info_t* res= new cdap_rib::res_info_t;
	// FIXME: change for real opcode
	res->result_ = -2;
	return res;
}

cdap_rib::res_info_t* RIBObj::stop(
		const std::string& name,
		const cdap_rib::SerializedObject &obj_req,
		cdap_rib::SerializedObject &obj_reply) {

	(void) name;
	(void) obj_req;
	(void) obj_reply;
	operation_not_supported();
	cdap_rib::res_info_t* res= new cdap_rib::res_info_t;
	// FIXME: change for real opcode
	res->result_ = -2;
	return res;
}

const std::string& RIBObj::get_class() const {

	return class_;
}

const std::string& RIBObj::get_name() const {

	return name_;
}

long RIBObj::get_instance() const {

	return instance_;
}

void RIBObj::operation_not_supported() {

	LOG_ERR("Operation not supported");
}


//Single RIBDaemon instance
static RIBDaemon* ribd = NULL;

//
// RIBDaemon (public) interface (proxy)
//

//Constructor (private)
RIBDaemonProxy::RIBDaemonProxy(RIBDaemon* ribd_):ribd(ribd_){};

// Register a RIB
void RIBDaemonProxy::registerRIB(RIB* rib){
	ribd->registerRIB(rib);
}

// List registered RIB versions
std::list<uint64_t> RIBDaemonProxy::listVersions(void){
	return ribd->listVersions();
}

// Retrieve a pointer to the RIB
RIB* RIBDaemonProxy::get(uint64_t version, std::string& ae_name){
	return ribd->get(version, ae_name);
}

// Unregister a RIB
RIB* RIBDaemonProxy::unregisterRIB(RIB* inst){
	return ribd->unregisterRIB(inst);
}

// Establish a CDAP connection to a remote RIB
cdap_rib::con_handle_t RIBDaemonProxy::remote_open_connection(
		const cdap_rib::vers_info_t &ver,
		const cdap_rib::src_info_t &src,
		const cdap_rib::dest_info_t &dest,
		const cdap_rib::auth_info &auth, int port_id){

	cdap_rib::con_handle_t handle =
		ribd->cdap_provider->remote_open_connection(ver, src, dest,
								auth,
								port_id);
	//TODO store?
	return handle;
}

// Close a CDAP connection to a remote RIB
int RIBDaemonProxy::remote_close_connection(unsigned int port){
	int res = ribd->cdap_provider->remote_close_connection(port);

	//TODO remove from storage?

	return res;
}

// Perform a create operation over an object of the remote RIB
int RIBDaemonProxy::remote_create(unsigned int port,
			  const cdap_rib::obj_info_t &obj,
			  const cdap_rib::flags_t &flags,
			  const cdap_rib::filt_info_t &filt){
	return ribd->cdap_provider->remote_create(port,	obj, flags, filt);
}

// Perform a delete operation over an object of the remote RIB
int RIBDaemonProxy::remote_delete(unsigned int port,
			  const cdap_rib::obj_info_t &obj,
			  const cdap_rib::flags_t &flags,
			  const cdap_rib::filt_info_t &filt){
	return ribd->cdap_provider->remote_delete(port, obj, flags, filt);
}

// Perform a read operation over an object of the remote RIB
int RIBDaemonProxy::remote_read(unsigned int port,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::flags_t &flags,
			const cdap_rib::filt_info_t &filt){
	return ribd->cdap_provider->remote_read(port, obj, flags, filt);
}

// Perform a cancel read operation over an object of the remote RIB
int RIBDaemonProxy::remote_cancel_read(unsigned int port,
			       const cdap_rib::flags_t &flags,
			       int invoke_id){
	return ribd->cdap_provider->remote_cancel_read(port, flags, invoke_id);
}

// Perform a write operation over an object of the remote RIB
int RIBDaemonProxy::remote_write(unsigned int port,
			 const cdap_rib::obj_info_t &obj,
			 const cdap_rib::flags_t &flags,
			 const cdap_rib::filt_info_t &filt){
	return ribd->cdap_provider->remote_write(port, obj, flags, filt);
}

// Perform a start operation over an object of the remote RIB
int RIBDaemonProxy::remote_start(unsigned int port,
			 const cdap_rib::obj_info_t &obj,
			 const cdap_rib::flags_t &flags,
			 const cdap_rib::filt_info_t &filt){
	return ribd->cdap_provider->remote_start(port, obj, flags, filt);
}

// Perform a stop operation over an object of the remote RIB
int RIBDaemonProxy::remote_stop(unsigned int port,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::flags_t &flags,
			const cdap_rib::filt_info_t &filt){
	return ribd->cdap_provider->remote_stop(port, obj, flags, filt);
}

//
// RIB library routines
//
void init(cacep::AppConHandlerInterface *app_con_callback,
					RIBOpsRespHandlers* remote_handlers,
					cdap_rib::cdap_params params){

	//Callbacks
	if(ribd){
		LOG_ERR("Double call to rina::rib::init()");
		throw Exception("Double call to rina::rib::init()");
	}

	//Initialize intializes the RIB daemon
	ribd = new RIBDaemon(app_con_callback, remote_handlers, params);
}

RIBDaemonProxy* RIBDaemonProxyFactory(){
	if(!ribd){
		LOG_ERR("RIB library not initialized! Call rib::init() method first");
		throw Exception("Double call to rina::rib::init()");

	}
	return new RIBDaemonProxy(ribd);
}

void fini(){
	delete ribd;
}


}  //namespace rib
}  //namespace rina
