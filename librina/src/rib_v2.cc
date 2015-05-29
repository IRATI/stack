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

#if 0
//
// Internals Wrap over a RIBObj
//
// This class encapsulate the internal state required for a RIBObj, which is
// deliverately not exposed to the RIB library user
//
class RIBObjWrap {

	friend class RIB;

protected:

	//Constructor&destructor
	RIBObjWrap(RIBObj* object, const std::string& fqn);
	~RIBObjWrap();

	// Add a child element
	bool add_child(RIBObjWrap* child);

	// Remove a child element
	bool remove_child(const std::string& name);

	// Reference to the object
	RIBObj* object;

	// Parent object, if any
	RIBObjWrap* parent;

	// List of childs
	std::list<RIBObjWrap*> children;

	// Fully qualifed name
	std::string fqn;

	// Read/write lock
	rina::ReadWriteLockable rwlock;
};

RIBObjWrap::RIBObjWrap(RIBObj *obj, const std::string& fqn_) :
							object(obj),
							fqn(fqn_){
}

RIBObjWrap::~RIBObjWrap(){
	//Ensure there is no other thread still using this
        rwlock.writelock();
}

bool RIBObjWrap::add_child(RIBObjWrap* child) {

	//Mutual exclusion
	WriteScopedLock wlock(rwlock);

	for (std::list<RIBObjWrap*>::iterator it = children.begin();
			it != children.end(); ++it) {
		if ((*it)->fqn.compare(child->fqn) == 0)
			return false;
	}

	children.push_back(child);
	child->parent = this;

	return true;
}

bool RIBObjWrap::remove_child(const std::string& fqn) {

	//Mutual exclusion
	WriteScopedLock wlock(rwlock);

	for (std::list<RIBObjWrap*>::iterator it = children.begin();
			it != children.end(); ++it) {
		if ((*it)->fqn.compare(fqn) == 0) {
			children.erase(it);
			return true;
		}
	}

	LOG_ERR("Unknown child object with object name: %s", fqn.c_str());
	return false;
}
#endif

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

template<typename T>
bool RIBSchema::validateAddObject(const RIBObj<T>* obj) {

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

//fwd decl
class RIBDaemon;

/// A simple RIB implementation, based on a hashtable of RIB objects
/// indexed by object name
class RIB {

public:

	///
	/// Constructor
	///
	/// @param schema Schema in which this RIB will be based
	/// @param cdap_provider CDAP provider
	///
	RIB(const RIBSchema *schema,
				cdap::CDAPProviderInterface *cdap_provider);

	/// Destroy RIB instance
	~RIB();

	/// Add an object to the rib
	///
	/// @param fqn Fully qualified name (e.g. /x/y/z)
	/// @param obj Pointer to a pointer of a RIB object. On success the 
	/// pointer will be set to NULL, as the RIBObj now will be handled by
	/// the library. On failure the object remains unaltered
	///
	/// @ret instance_id of the objc
	///
	template <typename T>
	int64_t add_obj(const std::string& fqn, RIBObj<T>** obj);

	//
	// Get the instance id of an object given a fully qualified name (fqn)
	//
	// @ret The object instance id or -1 if it does not exist
	//
	int64_t get_obj_inst_id(const std::string& fqn) {
		ReadScopedLock rlock(rwlock);
		return __get_obj_inst_id(fqn);
	};

	//
	// Get the fully qualified name given the instance id
	//
	// @ret The object fqn or "" if does not exist
	//
	std::string get_obj_fqn(const int64_t inst_id) {
		ReadScopedLock rlock(rwlock);
		return __get_obj_fqn(inst_id);
	};

	//
	// Get the class name given a fully qualified name (fqn)
	//
 	// @ret A string with the class name or an exception if the object
	//	does not exist
	//
	const std::string get_obj_class(int64_t instance_id) const;

	//
	// Get (a copy) of the user data from an object
	//
	// @param inst_id The instance id of the object
	//
	template <typename T>
	T get_obj_user_data(const int64_t inst_id);

	//
	// Set (copy) the user data to an object
	//
	// @param inst_id The instance id of the object
	// @param user_data_ The new user data (T)
	//
	template <typename T>
	void set_obj_user_data(const int64_t inst_id, T user_data_);

	///
	/// Get the RIB's path separator
	///
	char get_separator(void) const;

	///
	/// Get the parent's fully qualified name given a fqn of a child
	///
	/// @param fqn_child Fully qualified name of the child
	///
	std::string get_parent_fqn(const std::string& fqn_child) const;

	///
	/// Get the child's name (not fqn) given a fqn of the parent
	///
	/// @param fqn_parent Fully qualified name of the parent
	///
	std::string get_child_fqn(const std::string& fqn_parent) const;

	///
	/// Remove an object from the RIB by instance id
	///
	/// @ret On success 0, otherwise -1
	///
	inline int remove_obj(int64_t instance_id){
		return __remove_obj(instance_id);
	}

	///
	/// Remove an object from the RIB by instance name
	///
	/// @ret On success 0, otherwise -1
	///
	int remove_obj(const std::string& fqn) {
		return __remove_obj(get_obj_inst_id(fqn));
	}

	///
	/// Get RIB's version
	///
	const cdap_rib::vers_info_t& get_version() const;

protected:
	//
	// Incoming requests to the local RIB
	//
	void create_request(const cdap_rib::con_handle_t &con,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::filt_info_t &filt,
			int invoke_id);
	void delete_request(const cdap_rib::con_handle_t &con,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::filt_info_t &filt,
			int invoke_id);
	void read_request(const cdap_rib::con_handle_t &con,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::filt_info_t &filt,
			int invoke_id);
	void cancel_read_request(const cdap_rib::con_handle_t &con,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::filt_info_t &filt,
			int invoke_id);
	void write_request(const cdap_rib::con_handle_t &con,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::filt_info_t &filt,
			int invoke_id);
	void start_request(const cdap_rib::con_handle_t &con,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::filt_info_t &filt,
			int invoke_id);
	void stop_request(const cdap_rib::con_handle_t &con,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::filt_info_t &filt,
			int invoke_id);
private:

	// fqn <-> obj
	std::map<std::string, RIBObj_*> obj_name_map;

	// instance id <-> obj
	std::map<int64_t, RIBObj_*> obj_inst_map;

	// instance id <-> fqn
	std::map<int64_t, std::string> inst_name_map;

	// fqn <-> instance id
	std::map<std::string, int64_t> name_inst_map;


	// delegation cache: fqn <-> obj
	std::map<std::string, RIBObj_*> deleg_cache;

	//Schema
	const RIBSchema *schema;

	//Next id pointer
	int64_t next_inst_id;

	//Number of objects that delegate
	unsigned int num_of_deleg;

	//@internal only; must be called with the rwlock acquired
	RIBObj_* get_obj(int64_t inst_id);

	//@internal: must be called with the rwlock acquired
	int __remove_obj(int64_t inst_id);

	//@internal: must be called with the rwlock acquired
	int64_t __get_obj_inst_id(const std::string& fqn);

	//@internal: must be called with the rwlock acquired
	std::string __get_obj_fqn(const int64_t inst_id);

	//@internal Get a new (unused) instance id (strictly > 0);
	//must be called with the wlock acquired
	int64_t get_new_inst_id(void);

	//CDAP Provider
	cdap::CDAPProviderInterface *cdap_provider;

	//rwlock
	ReadWriteLockable rwlock;

	//RIBDaemon to access operations callbacks
	friend class RIBDaemon;
};

RIB::RIB(const RIBSchema *schema_,
				cdap::CDAPProviderInterface *cdap_provider_){

	schema = schema_;
	cdap_provider = cdap_provider_;
}

RIB::~RIB() {

	//Mutual exclusion
	WriteScopedLock wlock(rwlock);

	for (std::map<std::string, RIBObj_*>::iterator it = obj_name_map
			.begin(); it != obj_name_map.end(); ++it) {
		LOG_INFO("Object %s removed from the RIB",
				it->second->fqn.c_str());
		delete it->second;
	}
	obj_name_map.clear();
	obj_inst_map.clear();
}

void RIB::create_request(const cdap_rib::con_handle_t &con,
		const cdap_rib::obj_info_t &obj,
		const cdap_rib::filt_info_t &filt,
		int invoke_id) {

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
	RIBObj_* rib_obj = NULL;

	int64_t id = get_obj_inst_id(obj.name_);

	if(id)
		rib_obj = get_obj(id);

	//FIXME: this => father delegation????
	if (rib_obj == NULL) {
		std::string parent_name = get_parent_fqn(obj.name_);
		id = get_obj_inst_id(parent_name);
		rib_obj = get_obj(id);
	}else{
		//Verify if the class matches
		//FIXME
	}


	if(!rib_obj){
		//FIXME treat error => neither son nor father found
		return;
	}

	//Hold the readlock until the end
	rina::ReadScopedLock rlock(rib_obj->rwlock, false);

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
				invoke_id);
	} catch (Exception &e) {
		LOG_ERR("Unable to send the response");
	}
	delete res;
}

void RIB::delete_request(const cdap_rib::con_handle_t &con,
		const cdap_rib::obj_info_t &obj,
		const cdap_rib::filt_info_t &filt,
		int invoke_id) {

	(void) filt;
	// FIXME add res and flags
	cdap_rib::flags_t flags;

	//Mutual exclusion
	ReadScopedLock rlock(rwlock);

	int64_t id = get_obj_inst_id(obj.name_);
	RIBObj_* rib_obj = get_obj(id);

	if (rib_obj) {
		cdap_rib::res_info_t* res = rib_obj->delete_(obj.name_);
		try {
			cdap_provider->send_delete_result(con.port_, obj,
					flags, *res,
					invoke_id);
		} catch (Exception &e) {
			LOG_ERR("Unable to send the response");
		}
		delete res;
	}else{
		//TODO
		///So what?
	}
}
void RIB::read_request(const cdap_rib::con_handle_t &con,
		const cdap_rib::obj_info_t &obj,
		const cdap_rib::filt_info_t &filt,
		int invoke_id) {

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

	//Mutual exclusion
	ReadScopedLock rlock(rwlock);

	int64_t id = get_obj_inst_id(obj.name_);
	RIBObj_* rib_obj = get_obj(id);

	if (rib_obj) {
		res = rib_obj->read(
				obj.name_, obj_reply.value_);
	} else {
		res = new cdap_rib::res_info_t;
		res->result_ = -1;
	}
	try {
		cdap_provider->send_read_result(con.port_,
				obj_reply,
				flags, *res,
				invoke_id);
	} catch (Exception &e) {
		LOG_ERR("Unable to send the response");
	}
	delete res;
}
void RIB::cancel_read_request(
		const cdap_rib::con_handle_t &con,
		const cdap_rib::obj_info_t &obj,
		const cdap_rib::filt_info_t &filt, int invoke_id) {

	(void) filt;

	// FIXME add res and flags
	cdap_rib::flags_t flags;
	cdap_rib::res_info_t* res;

	//Mutual exclusion
	ReadScopedLock rlock(rwlock);

	int64_t id = get_obj_inst_id(obj.name_);
	RIBObj_* rib_obj = get_obj(id);

	if (rib_obj) {
		res = rib_obj->cancelRead(obj.name_);
	} else {
		res = new cdap_rib::res_info_t;
		res->result_ = -1;
	}

	try {
		cdap_provider->send_cancel_read_result(
				con.port_, flags, *res, invoke_id);
	} catch (Exception &e) {
		LOG_ERR("Unable to send the response");
	}
	delete res;
}
void RIB::write_request(const cdap_rib::con_handle_t &con,
		const cdap_rib::obj_info_t &obj,
		const cdap_rib::filt_info_t &filt,
		int invoke_id) {

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

	//Mutual exclusion
	ReadScopedLock rlock(rwlock);

	int64_t id = get_obj_inst_id(obj.name_);
	RIBObj_* rib_obj = get_obj(id);

	if (rib_obj) {
		res = rib_obj->write(
				obj.name_, obj.value_, obj_reply.value_);
	} else {
		res = new cdap_rib::res_info_t;
		res->result_ = -1;
	}
	try {
		cdap_provider->send_write_result(con.port_, flags,
				*res,
				invoke_id);
	} catch (Exception &e) {
		LOG_ERR("Unable to send the response");
	}
	delete res;
}
void RIB::start_request(const cdap_rib::con_handle_t &con,
		const cdap_rib::obj_info_t &obj,
		const cdap_rib::filt_info_t &filt,
		int invoke_id) {

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

	//Mutual exclusion
	ReadScopedLock rlock(rwlock);

	int64_t id = get_obj_inst_id(obj.name_);
	RIBObj_* rib_obj = get_obj(id);

	if (rib_obj) {
		res = rib_obj->start(
				obj.name_, obj.value_, obj_reply.value_);
	} else {
		res = new cdap_rib::res_info_t;
		res->result_ = -1;
	}
	try {
		cdap_provider->send_start_result(con.port_, obj,
				flags, *res,
				invoke_id);
	} catch (Exception &e) {
		LOG_ERR("Unable to send the response");
	}
	delete res;
}
void RIB::stop_request(const cdap_rib::con_handle_t &con,
		const cdap_rib::obj_info_t &obj,
		const cdap_rib::filt_info_t &filt,
		int invoke_id) {

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

	int64_t id = get_obj_inst_id(obj.name_);
	RIBObj_* rib_obj = get_obj(id);

	if (rib_obj) {
		res = rib_obj->stop(
				obj.name_, obj.value_, obj_reply.value_);
	} else {
		res = new cdap_rib::res_info_t;
		res->result_ = -1;
	}
	try {
		cdap_provider->send_stop_result(con.port_, flags,
				*res,
				invoke_id);
	} catch (Exception &e) {
		LOG_ERR("Unable to send the response");
	}
	delete res;
}


RIBObj_* RIB::get_obj(int64_t inst_id){
	if(obj_inst_map.find(inst_id) != obj_inst_map.end())
		return obj_inst_map[inst_id];
	else
		return NULL;
}

int64_t RIB::__get_obj_inst_id(const std::string& fqn){
	int64_t id = -1;

	if(name_inst_map.find(fqn) != name_inst_map.end())
		id = name_inst_map[fqn];

	return id;
}

std::string RIB::__get_obj_fqn(const int64_t inst_id) {
	std::string fqn("");

	if(inst_name_map.find(inst_id) != inst_name_map.end())
		fqn = inst_name_map[inst_id];

	return fqn;
}

int64_t RIB::get_new_inst_id(){
	for(;; ++next_inst_id){
		//Reuse ids; restart at 1
		if(next_inst_id < 1)
			next_inst_id = 1;

		if(obj_inst_map.find(next_inst_id) != obj_inst_map.end())
			break;
	}
	return next_inst_id;
}

template <typename T>
int64_t RIB::add_obj(const std::string& fqn, RIBObj<T>** obj_) {

	int64_t id, parent_id;
	std::string parent_fqn = get_parent_fqn(fqn);
	RIBObj_* obj;

	if(!(*obj_)){
		LOG_ERR("Unable to add object(%p) at '%s'; object is NULL!",
								*obj,
								fqn.c_str());
		return -1;
	}

	//Recover the non-templatized part
	obj = *obj_;

	//Mutual exclusion
	WriteScopedLock wlock(rwlock);

	//Check whether the father exists
	parent_id = __get_obj_inst_id(parent_fqn);
	if(parent_id == -1){
		LOG_ERR("Unable to add object(%p) at '%s'; parent does not exist!",
								*obj,
								fqn.c_str());
		return -1;
	}

	//Check if the object already exists
	id = __get_obj_inst_id(fqn);
	if(id == -1){
		LOG_ERR("Unable to add object(%p) at '%s'; an object of class '%s' already exists!",
							*obj,
							fqn.c_str(),
							get_obj_class(id).c_str());
		return -1;
	}

	//get a (free) instance id
	id = get_new_inst_id();

	//Add it and return
	obj_name_map[fqn] = obj;
	obj_inst_map[id] = obj;
	inst_name_map[id] = fqn;
	name_inst_map[fqn] = id;

	if(obj->delegates){
		//Increase counter number of num_of_deleg
		num_of_deleg++;
		//For consistency (delegation in delegation); clear cache
		deleg_cache.clear();
	}

	//Mark pointer as acquired and return
	*obj_ = NULL;
	LOG_DBG("Object '%s' of class '%s' succesfully added (id:'%" PRId64 "')",
								fqn.c_str(),
								obj->get_class().c_str(),
								id);

	return id;
}

int RIB::__remove_obj(int64_t inst_id) {

	RIBObj_* obj;

	//Mutual exclusion
	WriteScopedLock wlock(rwlock);

	obj = get_obj(inst_id);

	if(!obj){
		LOG_ERR("Unable to remove with instance id '%" PRId64  "'. Object does not exist!",
								inst_id);
		return -1;
	}

	//Remove from the maps
	std::string fqn = get_obj_fqn(inst_id);
	obj_inst_map.erase(inst_id);
	inst_name_map.erase(inst_id);
	name_inst_map.erase(fqn);
	obj_name_map.erase(fqn);

	//If there Remove from the cache
	if(obj->delegates){
		//Remove cached delegated objs
		deleg_cache.clear();
		num_of_deleg--;
	}

	LOG_DBG("Object '%s' of class '%s' succesfully removed (id:'%" PRId64 "')",
							fqn.c_str(),
							obj->get_class().c_str(),
							inst_id);

	//Delete object
	delete obj;

	return 0;
}

char RIB::get_separator() const {
	return schema->get_separator();
}

std::string RIB::get_parent_fqn(const std::string& fqn_child) const {

	size_t last_separator = fqn_child.find_last_of(schema->separator_,
			std::string::npos);
	if (last_separator == std::string::npos)
		return "";

	return fqn_child.substr(0, last_separator);

}

const cdap_rib::vers_info_t& RIB::get_version() const {
	return schema->get_version();
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
	/// Register a RIB to an AE
	///
	void registerRIB(RIB* rib, const std::string& ae_name);

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
	/// @ret A pointer to the RIB object or NULL.
	///
	RIB* get(uint64_t version, const std::string& ae_name);

	///
	/// Retrieve a list of pointers to the RIB of a certain version
	///
	/// @param version RIB version
	///
	/// @ret a list of pointers to RIB objects
	///
	std::list<RIB*> get(uint64_t version);

	///
	/// Retrieve a list of pointers to the RIB(s) registered in an AE
	///
	/// @param  RIB version
	///
	/// @ret a list of pointers to RIB objects
	///
	std::list<RIB*> get(const std::string& ae_name);


	/// Unregister a RIB
	///
	/// Unregisters a RIB from the library. This method does NOT
	/// destroy the RIB instance.
	///
	/// @ret On success, it returns the pointer to the RIB instance
	///
	RIB* unregisterRIB(RIB* rib, const std::string& ae_name);

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
			const cdap_rib::flags_t &flags, int invoke_id);
	void close_connection(const cdap_rib::con_handle_t &con,
			const cdap_rib::flags_t &flags, int invoke_id);

	void create_request(const cdap_rib::con_handle_t &con,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::filt_info_t &filt,
			int invoke_id);
	void delete_request(const cdap_rib::con_handle_t &con,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::filt_info_t &filt,
			int invoke_id);
	void read_request(const cdap_rib::con_handle_t &con,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::filt_info_t &filt,
			int invoke_id);
	void cancel_read_request(const cdap_rib::con_handle_t &con,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::filt_info_t &filt,
			int invoke_id);
	void write_request(const cdap_rib::con_handle_t &con,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::filt_info_t &filt,
			int invoke_id);
	void start_request(const cdap_rib::con_handle_t &con,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::filt_info_t &filt,
			int invoke_id);
	void stop_request(const cdap_rib::con_handle_t &con,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::filt_info_t &filt,
			int invoke_id);


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

	typedef std::pair<std::string, uint64_t> __ae_version_key_t;

	//(AE+version) <-> list of RIB
	std::map<__ae_version_key_t, RIB*> aeversion_rib_map;

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
}

//
/// Register a RIB
///
void RIBDaemon::registerRIB(RIB* rib, const std::string& ae_name){

	__ae_version_key_t key;

	key.first = ae_name;
	key.second = rib->get_version().version_;

	//Mutual exclusion
	WriteScopedLock wlock(rwlock);

	//Check first if it really exists
	if(aeversion_rib_map.find(key) != aeversion_rib_map.end()){
		LOG_ERR("Attempting to register an existing RIB: %p", rib);
		throw eRIBAlreadyRegistered();
	}

	aeversion_rib_map[key] = rib;
}

///
/// List registered RIB versions
///
std::list<uint64_t> RIBDaemon::listVersions(void){
	std::list<uint64_t> vers;
	uint64_t ver;
	std::map<__ae_version_key_t, RIB*>::const_iterator it;

	//Mutual exclusion
	ReadScopedLock rlock(rwlock);

	//Copy keys
	for(it = aeversion_rib_map.begin(); it != aeversion_rib_map.end();
									++it){
		ver = it->first.second;
		if (std::find(vers.begin(), vers.end(), ver) ==
								vers.end())
			vers.push_back(ver);
	}

	return vers;
}

RIB* RIBDaemon::get(const uint64_t ver, const std::string& ae_name){

	__ae_version_key_t key;

	key.first = ae_name;
	key.second = ver;
	std::map<__ae_version_key_t, RIB*>::const_iterator it;

	//Mutual exclusion
	ReadScopedLock rlock(rwlock);

	it = aeversion_rib_map.find(key);

	if(it == aeversion_rib_map.end())
		return NULL;

	return it->second;
}


RIB* RIBDaemon::getByPortId(const int port_id){

	//Mutual exclusion
	ReadScopedLock rlock(rwlock);

	if(port_id_rib_map.find(port_id) == port_id_rib_map.end())
		return NULL;
	return port_id_rib_map[port_id];
}

///
/// Unregister RIB
///
RIB* RIBDaemon::unregisterRIB(RIB* rib, const std::string& ae_name){

	__ae_version_key_t key;
	std::map<__ae_version_key_t, RIB*>::iterator it;

	key.first = ae_name;
	key.second = rib->get_version().version_;

	//Mutual exclusion
	WriteScopedLock wlock(rwlock);

	it = aeversion_rib_map.find(key);

	//If it is not registered throw an exception
	if(it == aeversion_rib_map.end()){
		LOG_ERR("RIB(%p) is not registred in AE %s", rib,
							ae_name.c_str());
		throw eRIBNotRegistered();
	}

	aeversion_rib_map.erase(key);

	return it->second;
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
		throw eRIBNotFound();
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
//
// Connection callbacks
//

void RIBDaemon::remote_open_connection_result(const cdap_rib::con_handle_t &con,
		const cdap_rib::res_info_t &res) {

	// FIXME remove invoke_id

	app_con_callback_->connectResult(res, con);
}

void RIBDaemon::open_connection(const cdap_rib::con_handle_t &con,
		const cdap_rib::flags_t &flags,
		int invoke_id) {

	// FIXME add result
	cdap_rib::result_info res;
	(void) res;
	(void) flags;
	app_con_callback_->connect(invoke_id, con);

	//The connect was successful store
	store_connection(con);

	cdap_provider->send_open_connection_result(con, res, invoke_id);
}

void RIBDaemon::remote_close_connection_result(const cdap_rib::con_handle_t &con,
		const cdap_rib::result_info &res) {

	app_con_callback_->releaseResult(res, con);
	remove_connection(con);
}

void RIBDaemon::close_connection(const cdap_rib::con_handle_t &con,
		const cdap_rib::flags_t &flags,
		int invoke_id) {

	// FIXME add result
	cdap_rib::result_info res;
	(void) res;
	app_con_callback_->release(invoke_id, con);
	cdap_provider->send_close_connection_result(con.port_, flags, res,
			invoke_id);
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
		int invoke_id) {

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
					invoke_id);
		} catch (Exception &e) {
			LOG_ERR("Unable to send the response");
		}
		return;
	}

	//Invoke
	rib->create_request(con, obj, filt, invoke_id);
}

void RIBDaemon::delete_request(const cdap_rib::con_handle_t &con,
		const cdap_rib::obj_info_t &obj,
		const cdap_rib::filt_info_t &filt,
		int invoke_id) {

	//TODO this is not safe if RIB instances can be deleted
	RIB* rib = getByPortId(con.port_);

	if(!rib){
		cdap_rib::res_info_t res;
		cdap_rib::flags_t flags;
		res.result_ = -1;
		try {
			cdap_provider->send_delete_result(con.port_, obj,
					flags, res,
					invoke_id);
		} catch (Exception &e) {
			LOG_ERR("Unable to send the response");
		}
		return;
	}

	//Invoke
	rib->delete_request(con, obj, filt, invoke_id);
}

void RIBDaemon::read_request(const cdap_rib::con_handle_t &con,
		const cdap_rib::obj_info_t &obj,
		const cdap_rib::filt_info_t &filt,
		int invoke_id) {

	//TODO this is not safe if RIB instances can be deleted
	RIB* rib = getByPortId(con.port_);

	if(!rib){
		cdap_rib::res_info_t res;
		cdap_rib::flags_t flags;
		res.result_ = -1;
		try {
			cdap_provider->send_read_result(con.port_, obj,
					flags, res,
					invoke_id);
		} catch (Exception &e) {
			LOG_ERR("Unable to send the response");
		}
		return;
	}

	//Invoke
	rib->read_request(con, obj, filt, invoke_id);
}
void RIBDaemon::cancel_read_request(
		const cdap_rib::con_handle_t &con,
		const cdap_rib::obj_info_t &obj,
		const cdap_rib::filt_info_t &filt, int invoke_id) {

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
					invoke_id);
		} catch (Exception &e) {
			LOG_ERR("Unable to send the response");
		}
		return;
	}

	//Invoke
	rib->cancel_read_request(con, obj, filt, invoke_id);
}
void RIBDaemon::write_request(const cdap_rib::con_handle_t &con,
		const cdap_rib::obj_info_t &obj,
		const cdap_rib::filt_info_t &filt,
		int invoke_id) {

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
					invoke_id);
		} catch (Exception &e) {
			LOG_ERR("Unable to send the response");
		}
		return;
	}

	//Invoke
	rib->write_request(con, obj, filt, invoke_id);
}
void RIBDaemon::start_request(const cdap_rib::con_handle_t &con,
		const cdap_rib::obj_info_t &obj,
		const cdap_rib::filt_info_t &filt,
		int invoke_id) {

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
					invoke_id);
		} catch (Exception &e) {
			LOG_ERR("Unable to send the response");
		}
		return;
	}

	//Invoke
	rib->start_request(con, obj, filt, invoke_id);
}
void RIBDaemon::stop_request(const cdap_rib::con_handle_t &con,
		const cdap_rib::obj_info_t &obj,
		const cdap_rib::filt_info_t &filt,
		int invoke_id) {

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
					invoke_id);
		} catch (Exception &e) {
			LOG_ERR("Unable to send the response");
		}
		return;
	}

	//Invoke
	rib->stop_request(con, obj, filt, invoke_id);
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

//RIBObj/RIBObj_
cdap_rib::res_info_t* RIBObj_::create(
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

cdap_rib::res_info_t* RIBObj_::delete_(const std::string& name) {

	(void) name;
	operation_not_supported();
	cdap_rib::res_info_t* res= new cdap_rib::res_info_t;
	// FIXME: change for real opcode
	res->result_ = -2;
	return res;
}

// FIXME remove name, it is not needed
cdap_rib::res_info_t* RIBObj_::read(
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

cdap_rib::res_info_t* RIBObj_::cancelRead(const std::string& name) {

	(void) name;
	operation_not_supported();
	cdap_rib::res_info_t* res= new cdap_rib::res_info_t;
	// FIXME: change for real opcode
	res->result_ = -2;
	return res;
}

cdap_rib::res_info_t* RIBObj_::write(
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

cdap_rib::res_info_t* RIBObj_::start(
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

cdap_rib::res_info_t* RIBObj_::stop(
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

void RIBObj_::operation_not_supported() {

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
void RIBDaemonProxy::registerRIB(RIB* rib, const std::string& ae_name){
	ribd->registerRIB(rib, ae_name);
}

// List registered RIB versions
std::list<uint64_t> RIBDaemonProxy::listVersions(void){
	return ribd->listVersions();
}

// Retrieve a pointer to the RIB
RIB* RIBDaemonProxy::get(uint64_t version, const std::string& ae_name){
	return ribd->get(version, ae_name);
}

// Unregister a RIB
RIB* RIBDaemonProxy::unregisterRIB(RIB* rib, const std::string& ae_name){
	return ribd->unregisterRIB(rib, ae_name);
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
