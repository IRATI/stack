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
	RIB(const RIBSchema *schema, RIBOpsRespHandlers* app_resp_callback,
				cdap::CDAPProviderInterface *cdap_provider);
	~RIB() throw ();

	/// Given an objectname of the form "substring\0substring\0...substring" locate
	/// the RIBObj that corresponds to it
	/// @param objectName
	/// @return
	RIBObj* getRIBObj(const std::string& clas,
			const std::string& name, bool check);
	RIBObj* getRIBObj(const std::string& clas, long instance,
			bool check);
	RIBObj* removeRIBObj(const std::string& name);
	RIBObj* removeRIBObj(long instance);
	char get_separator() const;
	void addRIBObj(RIBObj* rib_object);
	std::string get_parent_name(const std::string child_name) const;
	const cdap_rib::vers_info_t& get_version() const;

	//
	// CDAP operations
	//

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


	void remote_create_request(const cdap_rib::con_handle_t &con,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::filt_info_t &filt,
			int message_id);
	void remote_delete_request(const cdap_rib::con_handle_t &con,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::filt_info_t &filt,
			int message_id);
	void remote_read_request(const cdap_rib::con_handle_t &con,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::filt_info_t &filt,
			int message_id);
	void remote_cancel_read_request(const cdap_rib::con_handle_t &con,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::filt_info_t &filt,
			int message_id);
	void remote_write_request(const cdap_rib::con_handle_t &con,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::filt_info_t &filt,
			int message_id);
	void remote_start_request(const cdap_rib::con_handle_t &con,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::filt_info_t &filt,
			int message_id);
	void remote_stop_request(const cdap_rib::con_handle_t &con,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::filt_info_t &filt,
			int message_id);
private:
	std::map<std::string, RIBObjWrap*> obj_name_map;
	std::map<long, RIBObjWrap*> obj_inst_map;
	const RIBSchema *schema;

	RIBObjWrap* getInternalRIBObj(const std::string& name);
	RIBObjWrap* getInternalRIBObj(long instance);

	//Callbacks
	RIBOpsRespHandlers* app_resp_callback;

	//CDAP Provider
	cdap::CDAPProviderInterface *cdap_provider;

	//Mutex
	Lockable mutex;
};

//
// Callbacks from events coming from the CDAP provider
//

void RIB::remote_create_result(const cdap_rib::con_handle_t &con,
		const cdap_rib::obj_info_t &obj,
		const cdap_rib::res_info_t &res) {
	app_resp_callback->createResponse(res, obj, con);
}
void RIB::remote_delete_result(const cdap_rib::con_handle_t &con,
		const cdap_rib::res_info_t &res) {

	app_resp_callback->deleteResponse(res, con);
}
void RIB::remote_read_result(const cdap_rib::con_handle_t &con,
		const cdap_rib::obj_info_t &obj,
		const cdap_rib::res_info_t &res) {
	app_resp_callback->readResponse(res, obj, con);
}
void RIB::remote_cancel_read_result(
		const cdap_rib::con_handle_t &con,
		const cdap_rib::res_info_t &res) {

	app_resp_callback->cancelReadResponse(res, con);
}
void RIB::remote_write_result(const cdap_rib::con_handle_t &con,
		const cdap_rib::obj_info_t &obj,
		const cdap_rib::res_info_t &res) {
	app_resp_callback->writeResponse(res, obj, con);
}
void RIB::remote_start_result(const cdap_rib::con_handle_t &con,
		const cdap_rib::obj_info_t &obj,
		const cdap_rib::res_info_t &res) {
	app_resp_callback->startResponse(res, obj, con);
}
void RIB::remote_stop_result(const cdap_rib::con_handle_t &con,
		const cdap_rib::obj_info_t &obj,
		const cdap_rib::res_info_t &res) {
	app_resp_callback->stopResponse(res, obj, con);
}

void RIB::remote_create_request(const cdap_rib::con_handle_t &con,
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
		res = rib_obj->remoteCreate(
				obj.name_, obj.class_, obj.value_,
				obj_reply.value_);
	} else {
		res = new cdap_rib::res_info_t;
		res->result_ = -1;
	}
	try {
		cdap_provider->remote_create_response(con.port_,
				obj_reply,
				flags, *res,
				message_id);
	} catch (Exception &e) {
		LOG_ERR("Unable to send the response");
	}
	delete res;
}

void RIB::remote_delete_request(const cdap_rib::con_handle_t &con,
		const cdap_rib::obj_info_t &obj,
		const cdap_rib::filt_info_t &filt,
		int message_id) {

	(void) filt;
	// FIXME add res and flags
	cdap_rib::flags_t flags;

	RIBObj* rib_obj = getRIBObj(obj.class_, obj.name_,
			true);
	if (rib_obj) {
		cdap_rib::res_info_t* res = rib_obj->remoteDelete(obj.name_);
		try {
			cdap_provider->remote_delete_response(con.port_, obj,
					flags, *res,
					message_id);
		} catch (Exception &e) {
			LOG_ERR("Unable to send the response");
		}
		delete res;
	}
}
void RIB::remote_read_request(const cdap_rib::con_handle_t &con,
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
		res = ribObj->remoteRead(
				obj.name_, obj_reply.value_);
	} else {
		res = new cdap_rib::res_info_t;
		res->result_ = -1;
	}
	try {
		cdap_provider->remote_read_response(con.port_,
				obj_reply,
				flags, *res,
				message_id);
	} catch (Exception &e) {
		LOG_ERR("Unable to send the response");
	}
	delete res;
}
void RIB::remote_cancel_read_request(
		const cdap_rib::con_handle_t &con,
		const cdap_rib::obj_info_t &obj,
		const cdap_rib::filt_info_t &filt, int message_id) {

	(void) filt;

	// FIXME add res and flags
	cdap_rib::flags_t flags;
	cdap_rib::res_info_t* res;

	RIBObj* ribObj = getRIBObj(obj.class_, obj.name_, true);
	if (ribObj) {
		res = ribObj->remoteCancelRead(obj.name_);
	} else {
		res = new cdap_rib::res_info_t;
		res->result_ = -1;
	}

	try {
		cdap_provider->remote_cancel_read_response(
				con.port_, flags, *res, message_id);
	} catch (Exception &e) {
		LOG_ERR("Unable to send the response");
	}
	delete res;
}
void RIB::remote_write_request(const cdap_rib::con_handle_t &con,
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
		res = ribObj->remoteWrite(
				obj.name_, obj.value_, obj_reply.value_);
	} else {
		res = new cdap_rib::res_info_t;
		res->result_ = -1;
	}
	try {
		cdap_provider->remote_write_response(con.port_, flags,
				*res,
				message_id);
	} catch (Exception &e) {
		LOG_ERR("Unable to send the response");
	}
	delete res;
}
void RIB::remote_start_request(const cdap_rib::con_handle_t &con,
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
		res = ribObj->remoteStart(
				obj.name_, obj.value_, obj_reply.value_);
	} else {
		res = new cdap_rib::res_info_t;
		res->result_ = -1;
	}
	try {
		cdap_provider->remote_start_response(con.port_, obj,
				flags, *res,
				message_id);
	} catch (Exception &e) {
		LOG_ERR("Unable to send the response");
	}
	delete res;
}
void RIB::remote_stop_request(const cdap_rib::con_handle_t &con,
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
		res = ribObj->remoteStop(
				obj.name_, obj.value_, obj_reply.value_);
	} else {
		res = new cdap_rib::res_info_t;
		res->result_ = -1;
	}
	try {
		cdap_provider->remote_stop_response(con.port_, flags,
				*res,
				message_id);
	} catch (Exception &e) {
		LOG_ERR("Unable to send the response");
	}
	delete res;
}

//Class RIB
RIB::RIB(const RIBSchema *schema_, RIBOpsRespHandlers* app_resp_callback_,
				cdap::CDAPProviderInterface *cdap_provider_){

	schema = schema_;
	app_resp_callback = app_resp_callback_;
	cdap_provider = cdap_provider_;
}

RIB::~RIB() throw () {

	mutex.lock();
	for (std::map<std::string, RIBObjWrap*>::iterator it = obj_name_map
			.begin(); it != obj_name_map.end(); ++it) {
		LOG_INFO("Object %s removed from the RIB",
				it->second->object_->get_name().c_str());
		delete it->second;
	}
	obj_name_map.clear();
	obj_inst_map.clear();
	mutex.unlock();
}

RIBObj* RIB::getRIBObj(const std::string& clas,
		const std::string& name, bool check) {
	RIBObjWrap* rib_object = getInternalRIBObj(name);

	if (rib_object) {
		if (check
				&& rib_object->object_->get_class().compare(
					clas) != 0) {
			LOG_ERR("RIB object class does not match the user specified one");
			return NULL;
		}

		return rib_object->object_;
	} else {
		LOG_ERR("RIB object %s is not in the RIB", name.c_str());
	}
	return NULL;
}

RIBObj* RIB::getRIBObj(const std::string& clas, long instance,
		bool check) {

	RIBObjWrap* rib_object = getInternalRIBObj(instance);

	if (rib_object) {
		if (check
				&& rib_object->object_->get_class().compare(
					clas) != 0) {
			LOG_ERR("RIB object class does not match the user specified one");
			return NULL;
		}

		return rib_object->object_;
	} else {
		LOG_ERR("RIB object %d is not in the RIB", instance);
	}
	return NULL;
}

void RIB::addRIBObj(RIBObj* rib_object) {

	RIBObjWrap *parent = NULL;
	RIBObjWrap *obj = NULL;

	// Check if the parent exists
	std::string parent_name = get_parent_name(rib_object->get_name());
	if (!parent_name.empty()) {
		parent = getInternalRIBObj(parent_name);
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

	obj = getInternalRIBObj(rib_object->get_name());
	if (obj) {
		std::stringstream ss;
		ss << "Object with the same name (" << obj->object_->get_name()
			<< ") already exists in the RIB" << std::endl;
		throw Exception(ss.str().c_str());
	}
	obj = getInternalRIBObj(rib_object->get_instance());
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
	mutex.lock();
	obj_name_map[int_object->object_->get_name()] = int_object;
	obj_inst_map[int_object->object_->get_instance()] = int_object;
	mutex.unlock();
}

RIBObj* RIB::removeRIBObj(const std::string& name) {
	RIBObjWrap* rib_object = getInternalRIBObj(name);
	if (rib_object) {
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
		mutex.lock();
		obj_name_map.erase(rib_object->object_->get_name());
		obj_inst_map.erase(rib_object->object_->get_instance());
		mutex.unlock();
		LOG_DBG("Object %s removed from the RIB",
				rib_object->object_->get_name().c_str());
		return rib_object->object_;
	} else {
		LOG_ERR("RIB object %s is not in the RIB", name.c_str());
	}
	return NULL;
}

RIBObj * RIB::removeRIBObj(long instance) {
	RIBObjWrap* rib_object = getInternalRIBObj(instance);
	if (rib_object) {
		RIBObjWrap* parent = rib_object->parent_;
		parent->remove_child(rib_object->object_->get_name());
		mutex.lock();
		obj_name_map.erase(rib_object->object_->get_name());
		obj_inst_map.erase(instance);
		mutex.unlock();
	} else {
		LOG_ERR("RIB object %d is not in the RIB", instance);
	}

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

RIBObjWrap* RIB::getInternalRIBObj(const std::string& name) {
	std::string norm_name = name;
	norm_name.erase(std::remove_if(norm_name.begin(), norm_name.end(),
				::isspace),
			norm_name.end());

	std::map<std::string, RIBObjWrap*>::iterator it;

	mutex.lock();
	it = obj_name_map.find(norm_name);
	mutex.unlock();
	if (it == obj_name_map.end()) {
		return NULL;
	}

	return it->second;
}

RIBObjWrap* RIB::getInternalRIBObj(long instance) {
	std::map<long, RIBObjWrap*>::iterator it;

	mutex.lock();
	it = obj_inst_map.find(instance);
	mutex.unlock();

	if (it == obj_inst_map.end()) {
		return NULL;
	}

	return it->second;
}

///
/// RIBDaemon main class
///
class RIBDaemon : public cdap::CDAPCallbackInterface {

public:
	RIBDaemon(cacep::AppConHandlerInterface *app_con_callback,
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
	RIB* get(uint64_t version, std::string& ae_name);

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
	void open_connection_result(const cdap_rib::con_handle_t &con,
			const cdap_rib::result_info &res);
	void open_connection(const cdap_rib::con_handle_t &con,
			const cdap_rib::flags_t &flags, int message_id);
	void close_connection_result(const cdap_rib::con_handle_t &con,
			const cdap_rib::result_info &res);
	void close_connection(const cdap_rib::con_handle_t &con,
			const cdap_rib::flags_t &flags, int message_id);

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

	void remote_create_request(const cdap_rib::con_handle_t &con,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::filt_info_t &filt,
			int message_id);
	void remote_delete_request(const cdap_rib::con_handle_t &con,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::filt_info_t &filt,
			int message_id);
	void remote_read_request(const cdap_rib::con_handle_t &con,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::filt_info_t &filt,
			int message_id);
	void remote_cancel_read_request(const cdap_rib::con_handle_t &con,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::filt_info_t &filt,
			int message_id);
	void remote_write_request(const cdap_rib::con_handle_t &con,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::filt_info_t &filt,
			int message_id);
	void remote_start_request(const cdap_rib::con_handle_t &con,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::filt_info_t &filt,
			int message_id);
	void remote_stop_request(const cdap_rib::con_handle_t &con,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::filt_info_t &filt,
			int message_id);
	void remote_open_connection(const cdap_rib::vers_info_t &ver,
			const cdap_rib::src_info_t &src,
			const cdap_rib::dest_info_t &dest,
			const cdap_rib::auth_info &auth,
			int port);
private:

	/**
	* @internal Get the RIB by port_id.
	* TODO: this should be deprecated in favour of putting some state
	* in the (opaque pointer, sort of cookie)
	*/
	RIB* getByPortId(int port_id);

	cacep::AppConHandlerInterface *app_con_callback_;
	cdap::CDAPProviderInterface *cdap_provider;

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
		cdap_rib::cdap_params params) {

	app_con_callback_ = app_con_callback;

	//Initialize the parameters
	//add cdap parameters
	cdap::init(this, params.is_IPCP_);
	cdap_provider = cdap::getProvider();
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

RIB* RIBDaemon::get(uint64_t ver, std::string& ae_name){

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


RIB* RIBDaemon::getByPortId(int port_id){

	//Mutual exclusion
	ReadScopedLock rlock(rwlock);

	if(port_id_rib_map.find(port_id) == port_id_rib_map.end())
		return NULL;
	return port_id_rib_map[port_id];
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

void RIBDaemon::open_connection_result(const cdap_rib::con_handle_t &con,
		const cdap_rib::res_info_t &res) {

	// FIXME remove message_id

	app_con_callback_->connectResponse(res, con);
}

void RIBDaemon::open_connection(const cdap_rib::con_handle_t &con,
		const cdap_rib::flags_t &flags,
		int message_id) {

	// FIXME add result
	cdap_rib::result_info res;
	(void) res;
	(void) flags;
	app_con_callback_->connect(message_id, con);
	cdap_provider->open_connection_response(con, res, message_id);
}

void RIBDaemon::close_connection_result(const cdap_rib::con_handle_t &con,
		const cdap_rib::result_info &res) {

	app_con_callback_->releaseResponse(res, con);
}

void RIBDaemon::close_connection(const cdap_rib::con_handle_t &con,
		const cdap_rib::flags_t &flags,
		int message_id) {

	// FIXME add result
	cdap_rib::result_info res;
	(void) res;
	app_con_callback_->release(message_id, con);
	cdap_provider->close_connection_response(con.port_, flags, res,
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
		rib->remote_create_result(con, obj, res);
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
		rib->remote_delete_result(con, res);
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
		rib->remote_read_result(con, obj, res);
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
		rib->remote_cancel_read_result(con, res);
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
		rib->remote_write_result(con, obj, res);
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
		rib->remote_start_result(con, obj, res);
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
		rib->remote_stop_result(con, obj, res);
	} catch (Exception &e) {
		LOG_ERR("Unable to process stop result");
	}
}

//CDAP object operations
void RIBDaemon::remote_create_request(const cdap_rib::con_handle_t &con,
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
			cdap_provider->remote_create_response(con.port_,
					obj,
					flags, res,
					message_id);
		} catch (Exception &e) {
			LOG_ERR("Unable to send the response");
		}
		return;
	}

	//Invoke
	rib->remote_create_request(con, obj, filt, message_id);
}

void RIBDaemon::remote_delete_request(const cdap_rib::con_handle_t &con,
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
			cdap_provider->remote_delete_response(con.port_, obj,
					flags, res,
					message_id);
		} catch (Exception &e) {
			LOG_ERR("Unable to send the response");
		}
		return;
	}

	//Invoke
	rib->remote_delete_request(con, obj, filt, message_id);
}

void RIBDaemon::remote_read_request(const cdap_rib::con_handle_t &con,
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
			cdap_provider->remote_read_response(con.port_, obj,
					flags, res,
					message_id);
		} catch (Exception &e) {
			LOG_ERR("Unable to send the response");
		}
		return;
	}

	//Invoke
	rib->remote_read_request(con, obj, filt, message_id);
}
void RIBDaemon::remote_cancel_read_request(
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
			cdap_provider->remote_cancel_read_response(
					con.port_,
					flags, res,
					message_id);
		} catch (Exception &e) {
			LOG_ERR("Unable to send the response");
		}
		return;
	}

	//Invoke
	rib->remote_cancel_read_request(con, obj, filt, message_id);
}
void RIBDaemon::remote_write_request(const cdap_rib::con_handle_t &con,
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
			cdap_provider->remote_write_response(
					con.port_,
					flags, res,
					message_id);
		} catch (Exception &e) {
			LOG_ERR("Unable to send the response");
		}
		return;
	}

	//Invoke
	rib->remote_write_request(con, obj, filt, message_id);
}
void RIBDaemon::remote_start_request(const cdap_rib::con_handle_t &con,
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
			cdap_provider->remote_start_response(
					con.port_, obj,
					flags, res,
					message_id);
		} catch (Exception &e) {
			LOG_ERR("Unable to send the response");
		}
		return;
	}

	//Invoke
	rib->remote_start_request(con, obj, filt, message_id);
}
void RIBDaemon::remote_stop_request(const cdap_rib::con_handle_t &con,
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
			cdap_provider->remote_stop_response(
					con.port_,
					flags, res,
					message_id);
		} catch (Exception &e) {
			LOG_ERR("Unable to send the response");
		}
		return;
	}

	//Invoke
	rib->remote_stop_request(con, obj, filt, message_id);

}

void RIBDaemon::remote_open_connection(const cdap_rib::vers_info_t &ver,
		const cdap_rib::src_info_t &src,
		const cdap_rib::dest_info_t &dest,
		const cdap_rib::auth_info &auth,
		int port) {
	cdap_provider->open_connection(ver, src, dest, auth,
			port);
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

std::string RIBObj::get_displayable_value() {

	return "-";
}

bool RIBObj::createObject(const std::string& clas,
		const std::string& name,
		const void* value) {

	(void) clas;
	(void) name;
	(void) value;
	operation_not_supported();
	return false;
}

bool RIBObj::deleteObject(const void* value) {

	(void) value;
	operation_not_supported();
	return false;
}

RIBObj* RIBObj::readObject() {

	return this;
}

bool RIBObj::writeObject(const void* value) {

	(void) value;
	operation_not_supported();
	return false;
}

bool RIBObj::startObject(const void* object) {

	(void) object;
	operation_not_supported();
	return false;
}

bool RIBObj::stopObject(const void* object) {

	(void) object;
	operation_not_supported();
	return false;
}

cdap_rib::res_info_t* RIBObj::remoteCreate(
		const std::string& name, const std::string clas,
		const cdap_rib::SerializedObject &obj_req,
		cdap_rib::SerializedObject &obj_reply) {

	(void) name;
	(void) clas;
	(void) obj_req;
	(void) obj_reply;
	operation_not_supported();
	cdap_rib::res_info_t* res= new cdap_rib::res_info_t;
	// FIXME: change for real opcode
	res->result_ = -2;
	return res;
}

cdap_rib::res_info_t* RIBObj::remoteDelete(const std::string& name) {

	(void) name;
	operation_not_supported();
	cdap_rib::res_info_t* res= new cdap_rib::res_info_t;
	// FIXME: change for real opcode
	res->result_ = -2;
	return res;
}

// FIXME remove name, it is not needed
cdap_rib::res_info_t* RIBObj::remoteRead(
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

cdap_rib::res_info_t* RIBObj::remoteCancelRead(const std::string& name) {

	(void) name;
	operation_not_supported();
	cdap_rib::res_info_t* res= new cdap_rib::res_info_t;
	// FIXME: change for real opcode
	res->result_ = -2;
	return res;
}

cdap_rib::res_info_t* RIBObj::remoteWrite(
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

cdap_rib::res_info_t* RIBObj::remoteStart(
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

cdap_rib::res_info_t* RIBObj::remoteStop(
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
// RIB library routines
//
void init(cacep::AppConHandlerInterface *app_con_callback,
			cdap_rib::cdap_params params){

	//Callbacks
	if(ribd){
		LOG_ERR("Double call to rina::rib::init()");
		throw Exception("Double call to rina::rib::init()");
	}

	//Initialize intializes the RIB daemon
	ribd = new RIBDaemon(app_con_callback, params);
}

RIBDaemonProxy* getRIBDaemonProxy(){
	return new RIBDaemonProxy();
}

void fini(){
	delete ribd;
}


}  //namespace rib
}  //namespace rina
