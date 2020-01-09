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
#include <unistd.h>
#include <stdio.h>

#include <algorithm>
#include <map>
#define RINA_PREFIX "rib"
#include <librina/logs.h>
//FIXME iostream is only for debuging purposes
//#include <iostream>

#include "librina/rib_v2.h"
#include "librina/cdap_v2.h"
#include "librina/security-manager.h"

namespace rina {
namespace rib {

//
// CLASS RIBSchemaObject
//
RIBSchemaObject::RIBSchemaObject(const std::string& class_name,
				 const bool mandatory,
				 const unsigned max_objs)
{
	class_name_ = class_name;
	mandatory_ = mandatory;
	max_objs_ = max_objs;
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
// RIB schema class
//
class RIBSchema {

public:
	RIBSchema(const cdap_rib::vers_info_t& version,
					const char separator = '/',
					const std::string& root_name="");
	virtual ~RIBSchema();
	rib_schema_res ribSchemaDefContRelation(
			const std::string& cont_class_name,
			const std::string& class_name,
			const bool mandatory,
			const unsigned max_objs);
	char get_separator() const;
	std::string get_root_name() const;
	const cdap_rib::vers_info_t& get_version() const;

	const std::string& get_root_fqn(void) const{
		return root_fqn;
	}

	//This method IS thread safe
	void inc_ref_count(){
		WriteScopedLock wlock(rwlock);
		refs++;
	}

	//This method IS thread safe
	void dec_ref_count(){
		WriteScopedLock wlock(rwlock);
		if(refs > 0){
			refs--;
		}else{
			assert(0);
			LOG_ERR("Corrupted RIB schema (%p) ref counter",
									this);
		}
	}


	create_cb_t get_create_callback(const std::string& class_,
						const std::string& fqn_);

	void add_create_callback(const std::string& class_,
						const std::string& fqn_,
						create_cb_t cb);
private:
	bool validateAddObject(const RIBObj* obj);
	bool validateRemoveObject(const RIBObj* obj,
			const RIBObj* parent);
	const cdap_rib::vers_info_t version;
	std::map<std::string, RIBSchemaObject*> rib_schema;
	const char separator;
	const std::string root_name;

	//Root fqn
	std::string root_fqn;

	//Class name <-> create callback map
	std::map<std::string, create_cb_t> class_cb_map;

	//Key Classname+fqn
	typedef std::pair<std::string, std::string> cn_fqn_pair_t;

	// Classname+fqn <-> create callback map
	std::map<cn_fqn_pair_t, create_cb_t> class_fqn_cb_map;

	//number of RIBs using this schema
	unsigned int refs;

	rina::ReadWriteLockable rwlock;
};


RIBSchema::RIBSchema(const cdap_rib::vers_info_t &version_,
					const char separator_,
					const std::string& root_name_) :
							version(version_),
							separator(separator_),
							root_name(root_name_),
							refs(0){
	root_fqn = std::string("");
	root_fqn.push_back(separator);
}

RIBSchema::~RIBSchema() {}

rib_schema_res RIBSchema::ribSchemaDefContRelation(
		const std::string& cont_class_name,
		const std::string& class_name, const bool mandatory,
		const unsigned max_objs) {

	RIBSchemaObject *object = new RIBSchemaObject(class_name, mandatory,
			max_objs);
	std::map<std::string, RIBSchemaObject*>::iterator parent_it =
		rib_schema.find(cont_class_name);

	if (parent_it == rib_schema.end())
		return RIB_SCHEMA_FORMAT_ERR;

	std::pair<std::map<std::string, RIBSchemaObject*>::iterator, bool> ret =
		rib_schema.insert(
				std::pair<std::string, RIBSchemaObject*>(
					class_name, object));

	return (ret.second)? RIB_SUCCESS : RIB_SCHEMA_FORMAT_ERR;
}


bool RIBSchema::validateAddObject(const RIBObj* obj) {
	return true;
}

std::string RIBSchema::get_root_name() const {
	return root_name;
}

char RIBSchema::get_separator() const {
	return separator;
}

const cdap_rib::vers_info_t& RIBSchema::get_version() const {
	return version;
}

void RIBSchema::add_create_callback(const std::string& class_,
				    const std::string& fqn_,
				    create_cb_t cb)
{
	cn_fqn_pair_t key;

	if(class_ == "")
		throw eSchemaInvalidClass();

	//Mutual exclusion
	WriteScopedLock wlock(rwlock);

	if(fqn_ != "" ){
		key.first = class_;
		key.second = fqn_;

		if(class_fqn_cb_map.find(key) != class_fqn_cb_map.end())
			throw eSchemaCBRegExists();

		class_fqn_cb_map[key] = cb;
		LOG_DBG("Added create callback (specific) for class '%s' and path '%s'",
							class_.c_str(),
							fqn_.c_str());
	}else{
		if(class_cb_map.find(class_) != class_cb_map.end())
			throw eSchemaCBRegExists();

		class_cb_map[class_] = cb;
		LOG_DBG("Added create callback (generic) for class '%s'",
							class_.c_str());
	}
}

create_cb_t RIBSchema::get_create_callback(const std::string& class_,
						const std::string& fqn_){

	cn_fqn_pair_t key;

	//Mutual exclusion
	ReadScopedLock rlock(rwlock);

	//First check for a specific reg
	key.first = class_;
	key.second = fqn_;

	if(class_fqn_cb_map.find(key) != class_fqn_cb_map.end())
	{
		LOG_DBG("Found a callback for create operation of class '%s' and path '%s'",
								class_.c_str(),
								fqn_.c_str());
		return class_fqn_cb_map[key];
	}


	if(class_cb_map.find(class_) != class_cb_map.end())
	{
		LOG_DBG("Found a callback for create operation of class '%s' and path '%s'",
								class_.c_str(),
								fqn_.c_str());
		return class_cb_map[class_];
	}

	LOG_DBG("Callback not found for create operation of class '%s' and path '%s'",
							class_.c_str(),
							fqn_.c_str());
	return NULL;
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
	RIB(const rib_handle_t& handle_,
	    RIBSchema *const schema,
	    cdap::CDAPProviderInterface *cdap_provider,
	    ISecurityManager * sec_man,
	    const std::string& file_path = "");

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
	int64_t add_obj(const std::string& fqn, RIBObj** obj);

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
	std::string get_obj_class(const int64_t instance_id);

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
	inline void remove_obj(int64_t instance_id, bool force = false) {
		__remove_obj(instance_id, force);
	}

	///
	/// Remove an object from the RIB by instance name
	///
	/// @ret On success 0, otherwise -1
	///
	void remove_obj(const std::string& fqn, bool force = false) {
		__remove_obj(get_obj_inst_id(fqn), force);
	}

	///
	/// Get RIB's version
	///
	const cdap_rib::vers_info_t& get_version() const;

	std::list<RIBObjectData> get_all_rib_objects_data(
		const std::string& class_,
		const std::string& name);

	void set_security_manager(ISecurityManager * sec_man);

protected:
	//
	// Incoming requests to the local RIB
	//
	void create_request(const cdap_rib::con_handle_t &con,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::filt_info_t &filt,
			const cdap_rib::auth_policy_t& auth,
			const int invoke_id);
	void delete_request(const cdap_rib::con_handle_t &con,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::filt_info_t &filt,
			const cdap_rib::auth_policy_t& auth,
			const int invoke_id);
	void read_request(const cdap_rib::con_handle_t &con,
			  const cdap_rib::obj_info_t &obj,
			  const cdap_rib::filt_info_t &filt,
			  const cdap_rib::flags_t &flags,
			  const cdap_rib::auth_policy_t& auth,
			  const int invoke_id);
	void cancel_read_request(const cdap_rib::con_handle_t &con,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::filt_info_t &filt,
			const cdap_rib::auth_policy_t& auth,
			const int invoke_id);
	void write_request(const cdap_rib::con_handle_t &con,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::filt_info_t &filt,
			const cdap_rib::auth_policy_t& auth,
			const int invoke_id);
	void start_request(const cdap_rib::con_handle_t &con,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::filt_info_t &filt,
			const cdap_rib::auth_policy_t& auth,
			const int invoke_id);
	void stop_request(const cdap_rib::con_handle_t &con,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::filt_info_t &filt,
			const cdap_rib::auth_policy_t& auth,
			const int invoke_id);
private:

	// fqn <-> obj
	std::map<std::string, RIBObj*> obj_name_map;

	// instance id <-> obj
	std::map<int64_t, RIBObj*> obj_inst_map;

	// instance id <-> fqn
	std::map<int64_t, std::string> inst_name_map;

	// fqn <-> instance id
	std::map<std::string, int64_t> name_inst_map;

	// Children map instance id <-> std<int64_t> children
	std::map<int64_t, std::list<int64_t>*> obj_inst_child_map;

	// delegation cache: fqn <-> inst id
	std::map<std::string, int64_t> deleg_cache;

	//Schema
	RIBSchema *const schema;

	//Next id pointer
	int64_t next_inst_id;

	//Number of objects that delegate
	unsigned int num_of_deleg;

        //CDAP Provider
        cdap::CDAPProviderInterface *cdap_provider;

        //rwlock
        ReadWriteLockable rwlock;

        //Cache rwlock
        ReadWriteLockable cache_rwlock;

        //RIB handle (id)
        const rib_handle_t handle;

	void get_objects_to_operate(const int64_t object_id,
				    int scope,
				    char* filter,
				    std::list<std::pair<int, RIBObj*> >
	                            &objects);

	//return 0 if operation is allowed, negative number otherwise
	void check_operation_allowed(const cdap_rib::auth_policy_t & auth,
				     const cdap_rib::con_handle_t & con,
				     const cdap::cdap_m_t::Opcode opcode,
				     const std::string obj_name,
				     cdap_rib::res_info_t& res);

	//@internal only; must be called with the rwlock acquired
	RIBObj* get_obj(int64_t inst_id);

	//@internal: must be called with the rwlock acquired
	void __remove_obj(int64_t inst_id, bool force = false);

	//@internal: must be called with the rwlock acquired
	int64_t __get_obj_inst_id(const std::string& fqn);

	//@internal: must be called with the rwlock acquired
	std::string __get_obj_fqn(const int64_t inst_id);

	//@internal: validate an object name
	void __validate_fqn(const std::string& fqn);

	//@internal must be called with the rwlock acquired
	std::string __get_obj_class(const int64_t instance_id);

	//@internal Get a new (unused) instance id (strictly > 0);
	//must be called with the wlock acquired
	int64_t get_new_inst_id(void);

	int compareString(std::string a, std::string b);

	//RIBDaemon to access operations callbacks
	friend class RIBDaemon;

	//Instance of the security manager
	ISecurityManager * security_m;

	// The base path to export the RIB at the filesystem
	std::string base_file_path;
};

RIB::RIB(const rib_handle_t& handle_,
	 RIBSchema *const schema_,
	 cdap::CDAPProviderInterface *cdap_provider_,
	 ISecurityManager * sec_man,
	 const std::string& file_path) :
						schema(schema_),
						next_inst_id(1),
						num_of_deleg(0),
						cdap_provider(cdap_provider_),
						handle(handle_){

	std::stringstream root_fqn;
	std::stringstream ss;

	//Create root object
	RIBObj* root = new RootObj();

	// Root fqn
	root_fqn << schema->get_root_name() << schema->get_separator();

	// Fill in the stuf
	obj_name_map[root_fqn.str()] = root;
	obj_inst_map[RIB_ROOT_INST_ID] = root;
	inst_name_map[RIB_ROOT_INST_ID] = root_fqn.str();
	name_inst_map[root_fqn.str()] = RIB_ROOT_INST_ID;
	std::list<int64_t>* child_list = new std::list<int64_t>();
	obj_inst_child_map[RIB_ROOT_INST_ID] = child_list;
	security_m = sec_man;

	base_file_path = file_path;
	if (base_file_path != "") {
		createdir(base_file_path);
	}
}

RIB::~RIB() {
	std::list<int64_t> *child_list;
	int64_t inst_id;
	RIBObj* obj;
	std::map<int64_t, RIBObj*>::iterator it;
	std::stringstream ss;

	//Mutual exclusion
	WriteScopedLock wlock(rwlock);

	//Remove objects
	for(it = obj_inst_map.begin(); it != obj_inst_map.end(); it++){
		inst_id = it->first;
		obj = it->second;

		//If there, remove from the cache
		if(obj->delegates){
			//Remove cached delegated objs
			deleg_cache.clear();
			num_of_deleg--;
		}
		try{
			child_list = obj_inst_child_map[inst_id];
		}catch(...){
			LOG_ERR("Unable to recover the children list/parent"
				"children list for object '" PRId64 "';"
				"corrupted internal state!",inst_id);
			continue;
		}
		if (child_list != NULL){
			delete child_list;
		}
		delete obj;
	}

	//Remove temp file system with exported RIB if it exists
	if (base_file_path != "") {
		removedir_all(base_file_path);
	}

	//TODO: remove schema if allocated by us?
}

void RIB::check_operation_allowed(const cdap_rib::auth_policy_t & auth,
			          const cdap_rib::con_handle_t & con,
				  const cdap::cdap_m_t::Opcode opcode,
				  const std::string obj_name,
				  cdap_rib::res_info_t& res)
{
        if (!security_m) {
                LOG_DBG("RIB:: no security manager, positive check");
		res.code_ = cdap_rib::CDAP_SUCCESS;
		return;
	}

	ISecurityManagerPs *smps = dynamic_cast<ISecurityManagerPs *>(security_m->ps);
	assert(smps);
	smps->checkRIBOperation(auth, con, opcode, obj_name, res);

	return;
}

void RIB::create_request(const cdap_rib::con_handle_t &con,
			 const cdap_rib::obj_info_t &obj,
			 const cdap_rib::filt_info_t &filt,
			 const cdap_rib::auth_policy_t &auth,
			 const int invoke_id)
{
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
	cdap_rib::res_info_t res;

	check_operation_allowed(auth,
			        con,
				cdap::cdap_m_t::M_CREATE,
				obj.name_,
				res);

	if (res.code_ != cdap_rib::CDAP_SUCCESS) {
		try {
			cdap_provider->send_create_result(con,
							  obj_reply,
							  flags,
							  res,
							  invoke_id);
		} catch (...) {
			LOG_ERR("Unable to send response for invoke id %d",
					invoke_id);
		}

		return;
	}

	RIBObj* rib_obj = NULL;
	/* RAII scope for RIB scoped lock (read) */
	{
		//Mutual exclusion
		ReadScopedLock rlock(rwlock);

		int64_t id = get_obj_inst_id(obj.name_);
		if(id)
			rib_obj = get_obj(id);

		//Acquire the read lock over the object (make sure it is not
		//deleted while we process the operation)
		if(rib_obj)
			rib_obj->rwlock.readlock();
	} //RAII
	/* RAII scope for OBJ scoped lock(read) */
	if(rib_obj) {
		//Mutual exclusion
		ReadScopedLock rlock(rib_obj->rwlock, false);

		if (!rib_obj->delegates) {
			//If the object exists invoke the callback over the
			//object
			rib_obj->create(con, obj.name_, obj.class_, filt,
					invoke_id,
					obj.value_,
					obj_reply.value_,
					res);
		} else {
			cdap_rib::filt_info_t deleg_filt;
			DelegationObj *del_obj = (DelegationObj*) rib_obj;
			del_obj->forward_object(con,
						rina::cdap::cdap_m_t::M_CREATE,
						obj.name_,
						obj.class_,
						obj.value_,
						flags,
						deleg_filt,
						invoke_id);
		}
	} else {
		//Otherwise check the schema for a factory function
		std::string parent_name = get_parent_fqn(obj.name_);
		create_cb_t f = schema->get_create_callback(
							obj.class_,
							parent_name);
		//If the callback exists then call it otherwise
		//the operation is not supported
		if(f)
			(*f)(handle, con, obj.name_, obj.class_, filt,
						invoke_id,
						obj.value_,
						obj_reply,
						res);
		else
			res.code_ = cdap_rib::CDAP_OP_NOT_SUPPORTED;
	} //RAII

	if (res.code_ == cdap_rib::CDAP_PENDING ||
			invoke_id == 0)
		return;

	try {
		cdap_provider->send_create_result(con,
						  obj_reply,
						  flags,
						  res,
						  invoke_id);
	} catch (...) {
		LOG_ERR("Unable to send response for invoke id %d",
							invoke_id);
	}
}

void RIB::delete_request(const cdap_rib::con_handle_t &con,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::filt_info_t &filt,
			const cdap_rib::auth_policy_t &auth,
			const int invoke_id)
{
	// FIXME add res and flags
	cdap_rib::flags_t flags;
	cdap_rib::res_info_t res;
	RIBObj* rib_obj = NULL;
	bool delete_flag = false;
	int64_t id;

	check_operation_allowed(auth,
			        con,
				cdap::cdap_m_t::M_DELETE,
				obj.name_,
				res);

	if (res.code_ != cdap_rib::CDAP_SUCCESS) {
		try {
			cdap_provider->send_delete_result(con,
							  obj,
							  flags,
							  res,
							  invoke_id);
		} catch (Exception &e) {
			LOG_ERR("Unable to send response for invoke id %d",
								invoke_id);
		}

		return;
	}

	/* RAII scope for RIB scoped lock (read) */
	{
		//Mutual exclusion
		ReadScopedLock rlock(rwlock);

		id = get_obj_inst_id(obj.name_);
		rib_obj = get_obj(id);

		//Acquire the read lock over the object (make sure it is not
		//deleted while we process the operation)
		if(rib_obj)
			rib_obj->rwlock.readlock();
	} //RAII

	/* RAII scope for OBJ scoped lock(read) */
	if(rib_obj){
		//Mutual exclusion
		ReadScopedLock rlock(rib_obj->rwlock, false);

		delete_flag = rib_obj->delete_(con,
					       obj.name_,
					       obj.class_,
					       filt,
					       invoke_id,
					       res);
	}else{
		res.code_ = cdap_rib::CDAP_INVALID_OBJ;
	} //RAII

	//Remove the object if so specified by the callback
	if(delete_flag) {
		try {
			remove_obj(id);
		} catch (...) {
			LOG_ERR("Unable to remove object inst_id '%" PRId64 "' after callback",
					id);
		}
	}

	if (res.code_ == cdap_rib::CDAP_PENDING ||
			invoke_id == 0)
		return;

	try {
		cdap_provider->send_delete_result(con,
						  obj,
						  flags,
						  res,
						  invoke_id);
	} catch (Exception &e) {
		LOG_ERR("Unable to send response for invoke id %d",
							invoke_id);
	}
}

int RIB::compareString(std::string a, std::string b)
{
        int r = a.compare(b);
        if (r < 0)
                return -1;
        if (r > 0)
                return 1;
        return 0;
}

void RIB::read_request(const cdap_rib::con_handle_t &con,
		       const cdap_rib::obj_info_t &obj,
		       const cdap_rib::filt_info_t &filt,
		       const cdap_rib::flags_t &flags,
		       const cdap_rib::auth_policy_t &auth,
		       const int invoke_id)
{
	cdap_rib::flags flags_r;
	flags_r.flags_= cdap_rib::flags_t::NONE_FLAGS;
	//Reply object set to empty
	cdap_rib::obj_info_t obj_reply;
	obj_reply.name_ = obj.name_;
	obj_reply.class_ = obj.class_;
	obj_reply.inst_ = obj.inst_;
	obj_reply.value_.size_ = 0;
	obj_reply.value_.message_ = NULL;

	cdap_rib::res_info_t res;
	std::list<std::pair <int, RIBObj*> > objects;
        RIBObj* rib_obj = NULL;

	check_operation_allowed(auth,
			        con,
				cdap::cdap_m_t::M_READ,
				obj.name_,
				res);

	if (res.code_ != cdap_rib::CDAP_SUCCESS) {
		try {
			cdap_provider->send_read_result(con,
							obj_reply,
							flags,
							res,
							invoke_id);
		} catch (Exception &e) {
			LOG_ERR("Unable to send response for invoke id %d",
					invoke_id);
		}

		return;
	}

	/* RAII scope for RIB scoped lock (read) */
	{
		//Mutual exclusion
		ReadScopedLock rlock(rwlock);
		int64_t id = get_obj_inst_id(obj.name_);
		// Check if we have to delegate the call
		rib_obj = get_obj(id);

		//Get all objects affected by the operation
		get_objects_to_operate(id,
				       filt.scope_,
				       filt.filter_,
				       objects);
	} //RAII

	if(objects.size() == 0){
		if (invoke_id != 0) {
			res.code_ = cdap_rib::CDAP_INVALID_OBJ;

			try {
				cdap_provider->send_read_result(con,
								obj_reply,
								flags_r,
								res,
								invoke_id);
			} catch (Exception &e) {
				LOG_ERR("Unable to send response for invoke id %d",
						invoke_id);
			}
		}

		return;
	}

	std::list<std::pair<int, RIBObj*> >::iterator it;
	std::list<DelegationObj*> delegated_objs;
	unsigned int count = 0;
	for (it = objects.begin(); it != objects.end(); ++it) {
		rib_obj = it->second;
		count++;
		LOG_DBG("Processing read over object %s", rib_obj->fqn.c_str());
		//Mutual exclusion
		ReadScopedLock rlock(rib_obj->rwlock, false);

		rib_obj->read(con,
			      obj.name_,
			      obj.class_,
			      filt,
			      invoke_id,
			      obj_reply,
			      res);

		if (res.code_ == cdap_rib::CDAP_PENDING || invoke_id == 0)
			continue;

		// If the object is delegated
		int delegate = false;
		if(rib_obj->delegates)
		{
		        int rem_scope = filt.scope_ - it->first;
		        delegate = true;
		        std::string delegated_name;
		        //check if the request is for the delegated object or
		        //for its childs
		        switch(compareString(obj.name_,rib_obj->fqn))
		        {
		                // original name is equal to rib_obj name
		                case 0:
		                // original name is shorter to rib_obj name
		                case -1: if (rem_scope == 0)
                                        delegate = false;
                                else
                                        delegated_name = rib_obj->fqn + "/";
                                break;
		                        break;
		                // original name is longer to rib_obj name
		                case 1: delegated_name = obj.name_;
		                        break;

		        }
		        if (delegate)
		        {
		                rina::cdap_rib::filt_info_t deleg_filt;
		                deleg_filt.scope_ = rem_scope;
		                deleg_filt.filter_ = filt.filter_;
				DelegationObj *del_obj = (DelegationObj*)rib_obj;
                                if (count == objects.size())
                                        del_obj->last = true;
				del_obj->forward_object(con,
							rina::cdap::cdap_m_t::M_READ,
							delegated_name,
							rib_obj->class_name,
							obj.value_,
							flags,
							deleg_filt,
							invoke_id);

		        }
		}

		if(!delegate)
		{
			if (count == objects.size())
			{
				for(std::list<DelegationObj*>::iterator it =
				                delegated_objs.begin();
				                it!= delegated_objs.end();
				                ++it)
					(*it)->is_finished();
				flags_r.flags_ = cdap_rib::flags_t::NONE_FLAGS;
			} else
				flags_r.flags_ =
				                cdap_rib::flags_t::F_RD_INCOMPLETE;

			obj_reply.class_ = rib_obj->class_name;
			obj_reply.name_ = rib_obj->fqn;
			try {
				LOG_DBG("Sending read result for object %s with "
					"flags %d",
					 obj_reply.name_.c_str(),
					 flags_r.flags_);
				cdap_provider->send_read_result(con,
								obj_reply,
								flags_r,
								res,
								invoke_id);
			} catch (Exception &e) {
				LOG_ERR("Unable to send response for invoke id %d, problem was: %s",
					invoke_id,
					e.what());
			}
		} else
		{
			delegated_objs.push_back((DelegationObj*) rib_obj);
		}
	}
}

void RIB::cancel_read_request(const cdap_rib::con_handle_t &con,
			      const cdap_rib::obj_info_t &obj,
			      const cdap_rib::filt_info_t &filt,
			      const cdap_rib::auth_policy &auth,
			      const int invoke_id)
{
	// FIXME add res and flags
	cdap_rib::flags_t flags;
	cdap_rib::res_info_t res;
	RIBObj* rib_obj = NULL;

	check_operation_allowed(auth,
			        con,
				cdap::cdap_m_t::M_CANCELREAD,
				obj.name_,
				res);

	if (res.code_ != cdap_rib::CDAP_SUCCESS) {
		try {
			cdap_provider->send_cancel_read_result(con,
							       flags,
							       res,
							       invoke_id);
		} catch (Exception &e) {
			LOG_ERR("Unable to send response for invoke id %d",
								invoke_id);
		}

		return;
	}

	/* RAII scope for RIB scoped lock (read) */
	{
		//Mutual exclusion
		ReadScopedLock rlock(rwlock);

		int64_t id = get_obj_inst_id(obj.name_);
		rib_obj = get_obj(id);

		//Acquire the read lock over the object (make sure it is not
		//deleted while we process the operation)
		if(rib_obj)
			rib_obj->rwlock.readlock();
	} //RAII

	/* RAII scope for OBJ scoped lock(read) */
	if(rib_obj){
		//Mutual exclusion
		ReadScopedLock rlock(rib_obj->rwlock, false);

		rib_obj->cancelRead(con, obj.name_, obj.class_, filt,
							invoke_id, res);
	} else {
		res.code_ = cdap_rib::CDAP_INVALID_OBJ;
	} //RAII

	if (res.code_ == cdap_rib::CDAP_PENDING ||
			invoke_id == 0)
		return;

	try {
		cdap_provider->send_cancel_read_result(con,
						       flags,
						       res,
						       invoke_id);
	} catch (Exception &e) {
		LOG_ERR("Unable to send response for invoke id %d",
							invoke_id);
	}
}
void RIB::write_request(const cdap_rib::con_handle_t &con,
		const cdap_rib::obj_info_t &obj,
		const cdap_rib::filt_info_t &filt,
		const cdap_rib::auth_policy &auth,
		const int invoke_id) {

	// FIXME add res and flags
	cdap_rib::flags_t flags;

	//Reply object set to empty
	cdap_rib::obj_info_t obj_reply;
	obj_reply.name_ = obj.name_;
	obj_reply.class_ = obj.class_;
	obj_reply.inst_ = obj.inst_;
	obj_reply.value_.size_ = 0;
	obj_reply.value_.message_ = NULL;

	cdap_rib::res_info_t res;
	RIBObj* rib_obj = NULL;

	check_operation_allowed(auth,
			        con,
				cdap::cdap_m_t::M_WRITE,
				obj.name_,
				res);

	if (res.code_ != cdap_rib::CDAP_SUCCESS) {
		try {
			cdap_provider->send_write_result(con,
							flags,
							res,
							invoke_id);
		} catch (Exception &e) {
			LOG_ERR("Unable to send response for invoke id %d",
								invoke_id);
		}

		return;
	}

	/* RAII scope for RIB scoped lock (read) */
	{
		//Mutual exclusion
		ReadScopedLock rlock(rwlock);

		int64_t id = get_obj_inst_id(obj.name_);
		rib_obj = get_obj(id);

		//Acquire the read lock over the object (make sure it is not
		//deleted while we process the operation)
		if(rib_obj)
			rib_obj->rwlock.readlock();
	} //RAII

	/* RAII scope for OBJ scoped lock(read) */
	if(rib_obj){
		//Mutual exclusion
		ReadScopedLock rlock(rib_obj->rwlock, false);

		rib_obj->write(con, obj.name_, obj.class_, filt,
								invoke_id,
								obj.value_,
								obj_reply.value_,
								res);
	} else {
		res.code_ = cdap_rib::CDAP_INVALID_OBJ;
	} //RAII

	if (res.code_ == cdap_rib::CDAP_PENDING ||
			invoke_id == 0)
		return;

	try {
		cdap_provider->send_write_result(con,
						flags,
						res,
						invoke_id);
	} catch (Exception &e) {
		LOG_ERR("Unable to send response for invoke id %d",
							invoke_id);
	}
}
void RIB::start_request(const cdap_rib::con_handle_t &con,
		const cdap_rib::obj_info_t &obj,
		const cdap_rib::filt_info_t &filt,
		const cdap_rib::auth_policy &auth,
		const int invoke_id) {

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

	cdap_rib::res_info_t res;
	RIBObj* rib_obj = NULL;

	check_operation_allowed(auth,
			        con,
				cdap::cdap_m_t::M_START,
				obj.name_,
				res);

	if (res.code_ != cdap_rib::CDAP_SUCCESS) {
		try {
			cdap_provider->send_start_result(con,
							 obj,
							 flags,
							 res,
							 invoke_id);
		} catch (Exception &e) {
			LOG_ERR("Unable to send response for invoke id %d",
								invoke_id);
		}

		return;
	}

	/* RAII scope for RIB scoped lock (read) */
	{
		//Mutual exclusion
		ReadScopedLock rlock(rwlock);

		int64_t id = get_obj_inst_id(obj.name_);
		rib_obj = get_obj(id);

		//Acquire the read lock over the object (make sure it is not
		//deleted while we process the operation)
		if(rib_obj)
			rib_obj->rwlock.readlock();
	} //RAII

	/* RAII scope for OBJ scoped lock(read) */
	if(rib_obj){
		//Mutual exclusion
		ReadScopedLock rlock(rib_obj->rwlock, false);

		rib_obj->start(con, obj.name_, obj.class_, filt,
							invoke_id,
							obj.value_,
							obj_reply.value_,
							res);
	}else{
		res.code_ = cdap_rib::CDAP_INVALID_OBJ;
	} //RAII

	if (res.code_ == cdap_rib::CDAP_PENDING ||
			invoke_id == 0)
		return;

	try {
		cdap_provider->send_start_result(con,
						 obj,
						 flags,
						 res,
						 invoke_id);
	} catch (Exception &e) {
		LOG_ERR("Unable to send response for invoke id %d",
							invoke_id);
	}
}
void RIB::stop_request(const cdap_rib::con_handle_t &con,
		const cdap_rib::obj_info_t &obj,
		const cdap_rib::filt_info_t &filt,
		const cdap_rib::auth_policy &auth,
		const int invoke_id)
{
	// FIXME add res and flags
	cdap_rib::flags_t flags;

	//Reply object set to empty
	cdap_rib::obj_info_t obj_reply;
	obj_reply.name_ = obj.name_;
	obj_reply.class_ = obj.class_;
	obj_reply.inst_ = obj.inst_;
	obj_reply.value_.size_ = 0;
	obj_reply.value_.message_ = NULL;

	cdap_rib::res_info_t res;
	RIBObj* rib_obj = NULL;

	check_operation_allowed(auth,
			        con,
				cdap::cdap_m_t::M_STOP,
				obj.name_,
				res);

	if (res.code_ != cdap_rib::CDAP_SUCCESS) {
		try {
			cdap_provider->send_stop_result(con,
							flags,
							res,
							invoke_id);
		} catch (Exception &e) {
			LOG_ERR("Unable to send response for invoke id %d",
								invoke_id);
		}

		return;
	}

	/* RAII scope for RIB scoped lock (read) */
	{
		//Mutual exclusion
		ReadScopedLock rlock(rwlock);

		int64_t id = get_obj_inst_id(obj.name_);
		rib_obj = get_obj(id);

		//Acquire the read lock over the object (make sure it is not
		//deleted while we process the operation)
		if(rib_obj)
			rib_obj->rwlock.readlock();
	} //RAII

	/* RAII scope for OBJ scoped lock(read) */
	if(rib_obj){
		//Mutual exclusion
		ReadScopedLock rlock(rib_obj->rwlock, false);

		rib_obj->stop(con, obj.name_, obj.class_, filt,
								invoke_id,
								obj.value_,
								obj_reply.value_,
								res);
	} else {
		res.code_ = cdap_rib::CDAP_INVALID_OBJ;
	} //RAII

	if (res.code_ == cdap_rib::CDAP_PENDING ||
			invoke_id == 0)
		return;

	try {
		cdap_provider->send_stop_result(con,
						flags,
						res,
						invoke_id);
	} catch (Exception &e) {
		LOG_ERR("Unable to send response for invoke id %d",
							invoke_id);
	}
}

void RIB::get_objects_to_operate(const int64_t object_id,
			         int scope,
			         char * filter,
			         std::list<std::pair<int, RIBObj*> >
			         &objects)
{
	std::list<int64_t> *child_list = NULL;
	RIBObj *rib_obj = NULL;

	rib_obj = get_obj(object_id);
	if (!rib_obj)
		return;
	//TODO apply filter

	//Acquire the read lock over the object (make sure it is not
	//deleted while we process the operation)
	rib_obj->rwlock.readlock();
	std::pair<int, RIBObj*> pair (scope, rib_obj);
	objects.push_back(pair);

	if (scope == 0)
		return;

	child_list = obj_inst_child_map[object_id];
	if (!child_list || child_list->size() == 0)
		return;

	std::list<int64_t>::const_iterator it;
	for(it = child_list->begin(); it != child_list->end(); ++it)
		get_objects_to_operate(*it,
				       scope - 1,
				       filter,
				       objects);
}

RIBObj* RIB::get_obj(int64_t inst_id){
	if(obj_inst_map.find(inst_id) != obj_inst_map.end())
		return obj_inst_map[inst_id];
	else
		return NULL;
}

int64_t RIB::__get_obj_inst_id(const std::string& fqn){
	int64_t id = -1;

	if(name_inst_map.find(fqn) != name_inst_map.end())
		id = name_inst_map[fqn];

	//If there are delegated objects
	//Note: this block of code is specially polluted by RAII
	//I hate exceptions
	if(id == -1 && num_of_deleg > 0){

		/* rwlock RAII scope */ {

			ReadScopedLock rlock(cache_rwlock);

			//Check cache
			if(deleg_cache.find(fqn) != deleg_cache.end())
				id = deleg_cache[fqn];
		} //RAI

		//If found in cache return
		if(id != -1)
			return id;

		//If it is still not found, look recursively
		std::string tmp = fqn;
		std::string root_name = __get_obj_fqn(0);
		do{
			tmp = get_parent_fqn(tmp);
			if(name_inst_map.find(tmp) != name_inst_map.end())
				id = name_inst_map[tmp];
			if(id >= 0 || tmp == root_name)
				break;
		}while(1);

		//If not found or is root return
		if(id == -1)
			return id;

		//Check if it is a delegated obj
		RIBObj *obj = get_obj(id);
		if(!obj){
			assert(0); // neither this one
			return -1;
		}
		if(obj->delegates == false)
			return -1;

		//Since it is, add to cache
		/* rwlock RAII scope */ {
			WriteScopedLock rlock(cache_rwlock);
			deleg_cache[fqn] = id;
		} //RAI
	}

	return id;
}

std::string RIB::__get_obj_fqn(const int64_t inst_id) {
	std::string fqn("");

	if(inst_name_map.find(inst_id) != inst_name_map.end())
		fqn = inst_name_map[inst_id];

	return fqn;
}

int64_t RIB::get_new_inst_id(){

	int64_t curr = -1;

	for(;; ++next_inst_id){
		//Reuse ids; restart at 1
		if(next_inst_id < 1)
			next_inst_id = 1;

		//Set stop flag
		if(curr < 0)
			curr = next_inst_id;

		if(obj_inst_map.find(next_inst_id) == obj_inst_map.end())
			break;
	}
	return next_inst_id;
}

//Checks for fqn sanity.
void RIB::__validate_fqn(const std::string& fqn){

	char s = schema->get_separator();

	//Empty is an invalid name
	if(fqn == "") {
		LOG_ERR("Invalid empty object name.");
		throw eObjInvalidName();
	}

	//Ensure the fqn starts with the separator
	if(*fqn.begin() != s){
		LOG_ERR("Invalid object name '%s'. First character must be the RIB separator('%c')",
								fqn.c_str(),
								s);
		throw eObjInvalidName();
	}

	//Check for trailing separators
	if(*fqn.rbegin() == s){
		LOG_ERR("Invalid object name '%s'. Trailing RIB separator('%c') characters",
								fqn.c_str(),
								s);
		throw eObjInvalidName();
	}
}

int64_t RIB::add_obj(const std::string& fqn, RIBObj** obj_) {
	int64_t id, parent_id;
	std::string parent_fqn = get_parent_fqn(fqn);
	std::stringstream ss;

	//Note that obj_ cannot be NULL (checked by RIBDaemon)
	RIBObj* obj = *obj_;
	obj->rib = this;

	if(!obj){
		LOG_ERR("Unable to add object(%p) at '%s'; object is NULL!",
								obj,
								fqn.c_str());
		throw eObjInvalid();
	}
	LOG_DBG("Starting add object operation over RIB(%p), of object(%p) with fqn: '%s' (parent '%s')",
							this,
							obj,
							fqn.c_str(),
							parent_fqn.c_str());

	//Validate the name
	__validate_fqn(fqn);
	obj->fqn = fqn;

	//Mutual exclusion
	WriteScopedLock wlock(rwlock);

	//Check whether the father exists
	parent_id = __get_obj_inst_id(parent_fqn);
	if(parent_id == -1){
		LOG_ERR("Unable to add object(%p) at '%s'; parent does not exist!",
								obj,
								fqn.c_str());
		throw eObjNoParent();
	}

	//Check if the object already exists
	id = __get_obj_inst_id(obj->fqn);
	if(id != -1){
		LOG_ERR("Unable to add object(%p) at '%s'; an object of class '%s' already exists!",
							obj,
							fqn.c_str(),
							__get_obj_class(id).c_str());
		throw eObjExists();
	}

	//Allocate space
	std::list<int64_t>* child_list = new std::list<int64_t>();

	//get a (free) instance id
	id = get_new_inst_id();
	obj->parent_inst_id = parent_id;

	//Add it and return
	obj_name_map[fqn] = obj;
	obj_inst_map[id] = obj;
	inst_name_map[id] = fqn;
	name_inst_map[fqn] = id;
	obj_inst_child_map[id] = child_list;

	if(obj->delegates){
		//Increase counter number of num_of_deleg
		num_of_deleg++;
		//For consistency (delegation in delegation); clear cache
		deleg_cache.clear();
	}

	//Recover parent's children list  and add ourselves
	std::list<int64_t>* parent_child_list;
	try{
		parent_child_list = obj_inst_child_map[parent_id];
		if(parent_child_list == NULL)
			throw Exception();
	}catch(...){
		LOG_ERR("Unable to recover the children list for object '" PRId64  "'; corrupted internal state!",
						parent_id);
		assert(0);
		throw Exception("Corrupted internal state");
	}
	parent_child_list->push_back(id);

	LOG_DBG("Add object operation over RIB(%p), of object(%p) with fqn: '%s', succeeded. Instance id: '%" PRId64 "'",
								this,
								obj,
								fqn.c_str(),
								id);

	ss << base_file_path << fqn;
	if (base_file_path != "") {
		createdir(ss.str());
	}

	//Mark pointer as acquired and return
	*obj_ = NULL;

	return id;
}

void RIB::__remove_obj(int64_t inst_id, bool force)
{

	RIBObj* obj;
	std::list<int64_t> *child_list = NULL, *parent_child_list = NULL;
	std::list<int64_t> children;
	std::list<int64_t>::iterator it;
	int64_t parent_inst_id;
	std::stringstream ss;

	//Mutual exclusion
	rwlock.writelock();

	obj = get_obj(inst_id);
	if(!obj){
		LOG_ERR("Unable to remove with instance id '%" PRId64  "'. Object does not exist!",
								inst_id);
		rwlock.unlock();
		throw eObjDoesNotExist();
	}

	if(inst_id == RIB_ROOT_INST_ID){
		LOG_ERR("Unable to remove object with instance id '%" PRId64  "'; root can never be removed!",
								inst_id);
		rwlock.unlock();
		throw eObjInvalid();
	}

	parent_inst_id = obj->parent_inst_id;

	//Check first if it has children
	try{
		child_list = obj_inst_child_map[inst_id];
		if(child_list == NULL)
			throw Exception();
		parent_child_list = obj_inst_child_map[parent_inst_id];
		if(parent_child_list == NULL)
			throw Exception();
	}catch(...){
		LOG_ERR("Unable to recover the children list/parent children list for object '" PRId64  "'; corrupted internal state!",
							inst_id);
		rwlock.unlock();
		assert(0);
		throw Exception("Corrupted internal state");
	}

	if(child_list->size() > 0 && !force){
		LOG_ERR("Unable to remove object '" PRId64  "'; the object has children",
							inst_id);
		rwlock.unlock();
		throw eObjHasChildren();
	} else if (child_list->size() > 0) {
		//Make a copy of the list of children
		for(it=child_list->begin(); it != child_list->end(); ++it) {
			children.push_back(*it);
		}

		//Remove each children recursively
		for (it = children.begin(); it != children.end(); ++it) {
			rwlock.unlock();
			try {
				__remove_obj(*it, true);
			} catch (...) {
			}
			rwlock.writelock();
		}
	}

	//Check if we are in the parent's child list
	it = std::find(parent_child_list->begin(), parent_child_list->end(),
								inst_id);
	if(it == parent_child_list->end()){
		LOG_ERR("Parent's children list does not contain object '" PRId64  "'; corrupted internal state!",
							inst_id);
		rwlock.unlock();
		throw Exception("Corrupted internal state");
	}

	//Remove from the maps
	std::string fqn = __get_obj_fqn(inst_id);

	LOG_DBG("Removing object over RIB(%p) instance id: '%" PRId64 "' fqn: '%s'",
								this,
								inst_id,
								fqn.c_str());
	obj_inst_map.erase(inst_id);
	inst_name_map.erase(inst_id);
	name_inst_map.erase(fqn);
	obj_name_map.erase(fqn);
	obj_inst_child_map.erase(inst_id);

	//Remove ourselves from the parent's children list
	parent_child_list->erase(it);

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

	if (base_file_path != "") {
		ss << base_file_path << fqn;
		removedir_all(ss.str());
	}

	rwlock.unlock();

	//Delete object
	delete obj;
	delete child_list;
}

char RIB::get_separator() const {
	return schema->get_separator();
}

std::string RIB::get_parent_fqn(const std::string& fqn_child) const {

	size_t last_separator = fqn_child.find_last_of(schema->get_separator(),
			std::string::npos);

	//Treat the case /x (no root name)
	if(last_separator == 0)
		return fqn_child.substr(0,1);

	return fqn_child.substr(0, last_separator);
}

std::string RIB::get_obj_class(const int64_t inst_id){


	//Mutual exclusion
	ReadScopedLock rlock(rwlock);

	return __get_obj_class(inst_id);
}

std::string RIB::__get_obj_class(const int64_t inst_id){
	RIBObj* obj;

	obj = get_obj(inst_id);

	if(obj == NULL)
		throw eObjDoesNotExist();

	return obj->get_class();
}

const cdap_rib::vers_info_t& RIB::get_version() const {
	return schema->get_version();
}

std::list<RIBObjectData> RIB::get_all_rib_objects_data(
		const std::string& class_,
		const std::string& name)
{
	std::list<RIBObjectData> result;
	RIBObjectData data;
	unsigned n = name.size();

	for (std::map<std::string, RIBObj*>::iterator it = obj_name_map.begin();
			it != obj_name_map.end(); ++it) {
		data = it->second->get_object_data();
		if (class_.size() && class_ != data.class_)
			continue;
		if (n && (name[n-1] == '/' ? data.name_.compare(0, n, name)
					   : data.name_ != name))
			continue;
		if (it->first != "/")
			data.instance_ = __get_obj_inst_id(data.name_);
		result.push_back(data);
	}

	return result;
}

void RIB::set_security_manager(ISecurityManager * sec_man)
{
	security_m = sec_man;
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
	/// Create a RIB schema
	///
	///
	/// @throws eSchemaExists and Exception
	///
	void createSchema(const cdap_rib::vers_info_t& version,
			  const char separator = '/');
	///
	/// List registered RIB versions
	///
	std::list<cdap_rib::vers_info_t> listVersions(void);

	///
	/// Register a callback for CREATE operations
	///
	/// This method registers a callback method for CREATE operations. The
	/// callback can be registered either:
	///
	/// * For a class name *and* fully qualified name, in many locations in
	///   the tree as needed. (specific)
	/// * For a class name (generic)
	///
	/// Specific registrations always have preference over a generic.
	///
	/// @param version Schema version
	/// @param class_ Mandatory classhandle The handle of the RIB
	/// @param fqn Fully qualified name (position in the tree)
	/// @param cb Pointer to the callback method
	///
	/// @throws eSchemaNotFound, eSchemaInvalidClass and eSchemaCBRegExists
	///
	void addCreateCallbackSchema(const cdap_rib::vers_info_t& version,
				     const std::string& class_,
				     const std::string& fqn_,
				     create_cb_t cb);
	///
	/// Destroys a RIB schema
	///
	/// This method destroy a previously created schema. The schema shall
	/// not be currently used by any RIB instance or eSchemaInUse exception
	/// will be thrown.
	///
	/// @throws eSchemaInUse, eSchemaNotFound and Exception
	///
	void destroySchema(const cdap_rib::vers_info_t& version);

	///
	/// Create a RIB
	///
	/// This method creates an empty RIB and returns a handle to it. The
	/// RIB instance won't be operational until it has been associated to
	/// one or more Application Entities (AEs).
	///
	/// @ret The RIB handle
	/// @throws Exception on failure
	///
	rib_handle_t createRIB(const cdap_rib::vers_info_t& version,
			       const std::string& base_file_path = "");

	///
	/// Destroy a RIB instance
	///
	/// Destroys a previously created RIB instance. The instance shall not
	/// be assocated to any AE or it will throw eRIBInUse
	///
	/// @throws eRIBInUse, eRIBNotFound or Exception on failure
	void destroyRIB(const rib_handle_t& handle);

	///
	/// Associate a RIB to an Applicatin Entity (AE)
	///
	/// @throws eRIBAlreadyAssociated or Exception on failure
	///
	void associateRIBtoAE(const rib_handle_t& handle,
			      const std::string& ae_name);

	/// Deassociate RIB from an Application Entity
	///
	/// This method deassociates a RIB from an AE. This method does NOT
	/// destroy the RIB instance.
	///
	/// @throws eRIBNotFound, eRIBNotAssociated and Exception
	///
	void deassociateRIBfromAE(const rib_handle_t& handle,
				  const std::string& ae_name);

	///
	/// Retrieve the handle to a RIB
	///
	/// @param version RIB version
	/// @param Application Entity Name
	///
	/// @ret A handle to a RIB
	///
	rib_handle_t get(const cdap_rib::vers_info_t& version,
			 const std::string& ae_name);
	///
	/// Add an object to a RIB
	///
	/// This method attempts to add an object to the existing RIB (handle).
	/// On success, *obj is set to NULL and the callee shall not retain any
	/// copy of that pointer.
	///
	/// On failure the adequate exception is thrown, and no changes to obj
	/// will be made.
	///
	/// @param handle The handle of the RIB
	/// @param fqn Fully Qualified Name of the object
	/// @param obj A pointer (to a pointer) of the object to be added
	///
	/// @ret The instance id of the object created
	/// @throws eRIBNotFound, eObjExists
	///
	int64_t addObjRIB(const rib_handle_t& handle,
			  const std::string& fqn,
			  RIBObj** obj);

	///
	/// Retrieve the instance ID of an object given its fully
	/// qualified name.
	///
	/// @param handle The handle of the RIB
	/// @param fqn Fully Qualified Name of the object
	/// @param class__ Optional parameter. When defined (!=""), the class
	/// name of the object is checked to be strictly equl to class_
	///
	/// @ret The instance id of the object
	/// @throws eRIBNotFound, eObjDoesNotExist and eObjClassMismatch
	/// if class_ is defined.
	///
	int64_t getObjInstId(const rib_handle_t& handle,
			     const std::string& fqn,
			     const std::string& class_="");

	///
	/// Get parent's fully qualified name
	///
	/// @param handle The handle of the RIB
	/// @param fqn Fully Qualified Name of the child object
	///
	/// @ret Parent's Fqn
	/// @throws eRIBNotFound, eObjDoesNotExist
	///
	std::string getObjParentFqn(const rib_handle_t& handle,
				    const std::string& fqn);

	///
	/// Retrieve the fully qualified name given the instance ID of an
	/// object
	///
	/// @param handle The handle of the RIB
	/// @param inst_id Object's instance id
	/// @param class__ Optional parameter. When defined (!=""), the class
	///
	/// @ret The fully qualified name
	/// @throws eRIBNotFound, eObjDoesNotExist and eObjClassMismatch
	/// if class_ is defined
	///
	std::string getObjFqn(const rib_handle_t& handle,
			      const int64_t inst_id,
			      const std::string& class_="");
	///
	/// Retrieve the class name of an object given the instance ID.
	///
	/// @param handle The handle of the RIB
	/// @param inst_id Object's instance id
	/// @ret The class name
	/// @throws eRIBNotFound, eObjDoesNotExist and eObjClassMismatch
	///
	std::string getObjClass(const rib_handle_t& handle,
				const int64_t inst_id);
	///
	/// Remove an object to a RIB
	///
	/// This method removes an object previously added to the RIB
	///
	/// On failure the adequate exception is thrown and no changes will
	/// be performed in the RIB.
	///
	/// @param handle The handle of the RIB
	/// @param inst_id The object instance ID
	/// @param force if true, remove object and all children objects
	///
	/// @throws eRIBNotFound, eObjDoesNotExist
	///
	void removeObjRIB(const rib_handle_t& handle,
			  const int64_t,
			  bool force = false);

	//@internal: for testing purposes only)
	void __set_cdap_provider(cdap::CDAPProviderInterface* p){
		cdap_provider = p;
	}

	std::list<RIBObjectData> get_rib_objects_data(
		const rib_handle_t& handle,
		const std::string& class_,
		const std::string& name);

	int set_security_manager(ApplicationEntity * sec_man);

	///
	/// Perform an operation on a remote object. If resp_handler
	/// is not null, the response will be handled by him.
	///
	int remote_operation(const cdap_rib::con_handle_t& con,
			     const cdap::CDAPMessage::Opcode opcode,
			     const cdap_rib::obj_info_t &obj,
			     const cdap_rib::flags_t &flags,
			     const cdap_rib::filt_info_t &filt,
			     RIBOpsRespHandler * resp_handler);

	void destroy_cdap_connection(int port_id);

protected:
	//
	// CDAP provider callbacks
	//

	//Remote
	void remote_open_connection_result(cdap_rib::con_handle_t &con,
					   const cdap::CDAPMessage& message);
	void remote_close_connection_result(const cdap_rib::con_handle_t &con,
					    const cdap_rib::result_info &res);
	void remote_create_result(const cdap_rib::con_handle_t &con,
				  const cdap_rib::obj_info_t &obj,
				  const cdap_rib::res_info_t &res,
				  const int invoke_id);
	void remote_delete_result(const cdap_rib::con_handle_t &con,
				  const cdap_rib::obj_info_t &obj,
				  const cdap_rib::res_info_t &res,
				  const int invoke_id);
	void remote_read_result(const cdap_rib::con_handle_t &con,
				const cdap_rib::obj_info_t &obj,
				const cdap_rib::res_info_t &res,
				const cdap_rib::flags_t &flags,
				const int invoke_id);
	void remote_cancel_read_result(const cdap_rib::con_handle_t &con,
				       const cdap_rib::res_info_t &res,
				       const int invoke_id);
	void remote_write_result(const cdap_rib::con_handle_t &con,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::res_info_t &res,
			const int invoke_id);
	void remote_start_result(const cdap_rib::con_handle_t &con,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::res_info_t &res,
			const int invoke_id);
	void remote_stop_result(const cdap_rib::con_handle_t &con,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::res_info_t &res,
			const int invoke_id);


	//Local
	void open_connection(cdap_rib::con_handle_t &con,
			     const cdap::CDAPMessage& message);
	void close_connection(const cdap_rib::con_handle_t &con,
			      const cdap_rib::flags_t &flags,
			      const int invoke_id);
	void process_authentication_message(const cdap::CDAPMessage& message,
					    const cdap_rib::con_handle_t &con);
	void create_request(const cdap_rib::con_handle_t &con,
			    const cdap_rib::obj_info_t &obj,
			    const cdap_rib::filt_info_t &filt,
			    const cdap_rib::auth_policy_t& auth,
			    const int invoke_id);
	void delete_request(const cdap_rib::con_handle_t &con,
			    const cdap_rib::obj_info_t &obj,
			    const cdap_rib::filt_info_t &filt,
			    const cdap_rib::auth_policy_t& auth,
			    const int invoke_id);
	void read_request(const cdap_rib::con_handle_t &con,
			  const cdap_rib::obj_info_t &obj,
			  const cdap_rib::filt_info_t &filt,
			  const cdap_rib::flags_t &flags,
			  const cdap_rib::auth_policy_t& auth,
			  const int invoke_id);
	void cancel_read_request(const cdap_rib::con_handle_t &con,
				 const cdap_rib::obj_info_t &obj,
				 const cdap_rib::filt_info_t &filt,
				 const cdap_rib::auth_policy_t& auth,
				 const int invoke_id);
	void write_request(const cdap_rib::con_handle_t &con,
			   const cdap_rib::obj_info_t &obj,
			   const cdap_rib::filt_info_t &filt,
			   const cdap_rib::auth_policy_t& auth,
			   const int invoke_id);
	void start_request(const cdap_rib::con_handle_t &con,
			   const cdap_rib::obj_info_t &obj,
			   const cdap_rib::filt_info_t &filt,
			   const cdap_rib::auth_policy_t& auth,
			   const int invoke_id);
	void stop_request(const cdap_rib::con_handle_t &con,
			  const cdap_rib::obj_info_t &obj,
			  const cdap_rib::filt_info_t &filt,
			  const cdap_rib::auth_policy_t& auth,
			  const int invoke_id);

	//
	//Incoming connections (proxy only)
	//
	void store_connection(const cdap_rib::con_handle_t& con);
	void remove_connection(const cdap_rib::con_handle_t& con);

private:

	//Friendship
	friend class RIBDaemonProxy;

	///
	/// @internal Get the RIB by port_id.
	/// @warning This method shall be called with the r or wrlock acquired
	///
	RIB* getByPortId(const int port_id);

	///
	/// @internal Allocate a new handle
	/// @warning This method shall be called with the wrlock acquired
	///
	int64_t get_new_handle(void);

	///
	/// @internal Get the RIB pointer given the handle
	/// @warning This method shall be called with the r or wlock acquired
	///
	RIB* getRIB(const rib_handle_t& handle);

	///
	/// @internal get the response handler associated to the invoke_id
	///
	RIBOpsRespHandler * check_rib_and_get_response_handler(const cdap_rib::con_handle_t &con,
							       const int invoke_id,
						  	       bool remove);

	cacep::AppConHandlerInterface *app_con_callback_;
	cdap::CDAPProviderInterface *cdap_provider;

	/// CDAP Message handlers that have sent a CDAP message and are waiting for a reply
	ThreadSafeMapOfPointers<int, RIBOpsRespHandler> response_handlers;

	//Key type definition
	typedef std::pair<std::string, uint64_t> __ae_version_key_t;

	//Handle <-> schema
	std::map<rib_handle_t, RIB*> handle_rib_map;

	//Version <-> schema
	std::map<uint64_t, RIBSchema*> ver_schema_map;

	//(AE+version) <-> list of RIB
	std::map<__ae_version_key_t, RIB*> aeversion_rib_map;

	//Port id <-> RIB
	std::map<int, RIB*> port_id_rib_map;

	//Next handle possibly available
	int64_t next_handle_id;

	/**
	 * read/write lock
	 */
	ReadWriteLockable rwlock;

	//Pointer to security manager of the Application Process
	ISecurityManager * security_m;
};

//
// Constructors and destructors
//

RIBDaemon::RIBDaemon(cacep::AppConHandlerInterface *app_con_callback,
		     cdap_rib::cdap_params params) : next_handle_id(1)
{
	app_con_callback_ = app_con_callback;
	security_m = NULL;

	//Initialize the parameters
	//add cdap parameters
	cdap::init(this, params.syntax, params.ipcp);
	cdap_provider = cdap::getProvider();
}

RIBDaemon::~RIBDaemon() {
	// Delete all RIBS
	for(std::map<rib_handle_t, RIB*>::iterator it = handle_rib_map.begin();
			it != handle_rib_map.end(); it++){
		if(it->second)
			delete it->second;
	}

	// Delete all schemas
	for(std::map<uint64_t, RIBSchema*>::iterator it = ver_schema_map.begin();
			it != ver_schema_map.end(); it++){
		if(it->second)
			delete it->second;
	}
	cdap::fini();
}

int64_t RIBDaemon::get_new_handle(void){

	int64_t curr = -1;

	for(;; ++next_handle_id){
		//Reuse ids; restart at 1
		if(next_handle_id < 1)
			next_handle_id = 1;

		//Check whether we have checked all possible ids
		if(curr == next_handle_id)
			return -1;

		//Set stop flag
		if(curr < 0)
			curr = next_handle_id;

		if(handle_rib_map.find(next_handle_id) == handle_rib_map.end())
			break;
	}

	return next_handle_id;
}


RIB* RIBDaemon::getRIB(const rib_handle_t& handle){
	std::map<rib_handle_t, RIB*>::iterator it;

	it = handle_rib_map.find(handle);

	if(it == handle_rib_map.end())
		return NULL;

	return it->second;
}

RIBOpsRespHandler * RIBDaemon::check_rib_and_get_response_handler(const cdap_rib::con_handle_t &con,
								  const int invoke_id,
								  bool remove)
{
	RIBOpsRespHandler * handler;
	RIB * rib;

	//TODO this is not safe if RIB instances can be deleted
	rib = getByPortId(con.port_id);
	if(!rib) {
		LOG_WARN("Could not find RIB associated to handle %u",
				con.port_id);
		return NULL;
	}

	if (remove)
		handler = response_handlers.erase(invoke_id);
	else
		handler = response_handlers.find(invoke_id);

	if (!handler)
		LOG_WARN("Could not find response handler associated to invoke_id %d",
			 invoke_id);

	return handler;
}

// Associate a RIB to an AE
void RIBDaemon::createSchema(const cdap_rib::vers_info_t& version,
							const char separator){

	uint64_t ver = version.version_;
	std::map<uint64_t, RIBSchema*>::iterator it;

	//Mutual exclusion
	WriteScopedLock wlock(rwlock);

	//Find schema
	it = ver_schema_map.find(ver);

	if(it != ver_schema_map.end()){
		LOG_ERR("Schema version '%" PRIu64 "' exists", ver);
		throw eSchemaExists();
	}

	//Create the schema
	RIBSchema* schema = new RIBSchema(version, separator);
	ver_schema_map[ver] = schema;
}

void RIBDaemon::addCreateCallbackSchema(const cdap_rib::vers_info_t& version,
					const std::string& class_,
					const std::string& fqn_,
					create_cb_t cb)
{
	uint64_t ver = version.version_;
	std::map<uint64_t, RIBSchema*>::iterator it;

	//Mutual exclusion
	ReadScopedLock rlock(rwlock);

	//Find schema
	it = ver_schema_map.find(ver);

	if(it == ver_schema_map.end()){
		LOG_ERR("Schema version '%" PRIu64 "' not found. Create a schema first.",
								ver);
		throw eSchemaNotFound();
	}

	it->second->add_create_callback(class_, fqn_, cb);
}

void RIBDaemon::destroySchema(const cdap_rib::vers_info_t& version){
	throw eNotImplemented();
}


rib_handle_t RIBDaemon::createRIB(const cdap_rib::vers_info_t& version,
				  const std::string& base_file_path){

	rib_handle_t handle;
	uint64_t ver = version.version_;
	std::map<uint64_t, RIBSchema*>::iterator it;

	//Mutual exclusion
	WriteScopedLock wlock(rwlock);

	//Find schema
	it = ver_schema_map.find(ver);

	if(it == ver_schema_map.end()){
		LOG_ERR("Schema version '%" PRIu64 "' not found. Create a schema first.",
								ver);
		throw eSchemaNotFound();
	}

	//Get a new (unique) handle
	handle = get_new_handle();
	if(handle < 0){
		LOG_ERR("Could not retrieve a valid handle for RIB creation of %" PRIu64 "'.",
								ver);

		throw Exception("Invalid RIB handle");
	}

	//Store&inc schema count ref
	RIB* rib = new RIB(handle, it->second, cdap_provider, security_m, base_file_path);
	handle_rib_map[handle] = rib;
	it->second->inc_ref_count();

	return handle;
}

rib_handle_t RIBDaemon::get(const cdap_rib::vers_info_t& v,
						const std::string& ae_name){

	__ae_version_key_t key;

	key.first = ae_name;
	key.second = v.version_;
	std::map<__ae_version_key_t, RIB*>::const_iterator it;

	//Mutual exclusion
	ReadScopedLock rlock(rwlock);

	it = aeversion_rib_map.find(key);

	if(it == aeversion_rib_map.end())
		throw eRIBNotFound();

	return it->second->handle;
}

void RIBDaemon::destroyRIB(const rib_handle_t& handle){
	//Retreive the RIB
	RIB* rib = getRIB(handle);
	if (rib == NULL)
		throw eRIBNotFound();
	delete rib;
	handle_rib_map.erase(handle);
}

void RIBDaemon::associateRIBtoAE(const rib_handle_t& handle,
						const std::string& ae_name){

	__ae_version_key_t key;

	//Mutual exclusion
	WriteScopedLock wlock(rwlock);

	//Retreive the RIB
	RIB* rib = getRIB(handle);

	if(rib == NULL){
		LOG_ERR("Could not find RIB for handle %" PRId64 ". Already deleted?",
								handle);
		throw eRIBNotFound();
	}

	//Prepare the key
	key.first = ae_name;
	key.second = rib->get_version().version_;

	//Check first if registration exists
	if(aeversion_rib_map.find(key) != aeversion_rib_map.end()){
		LOG_ERR("Cannot associate RIB '%" PRId64 "' (version: '%" PRId64 "') to AE '%s'; an association with the same version already exists!",
						handle,
						rib->get_version().version_,
						ae_name.c_str());
		throw eRIBAlreadyAssociated();
	}

	aeversion_rib_map[key] = rib;
}

///
/// List registered RIB versions
///
std::list<cdap_rib::vers_info_t> RIBDaemon::listVersions(void){
	std::list<cdap_rib::vers_info_t> vers;
	cdap_rib::vers_info_t ver;
	std::map<uint64_t, RIBSchema*>::const_iterator it;

	//Mutual exclusion
	ReadScopedLock rlock(rwlock);

	//Copy keys
	for(it = ver_schema_map.begin(); it != ver_schema_map.end();
									++it){
		ver.version_ = it->first;
		vers.push_back(ver);
	}

	return vers;
}

RIB* RIBDaemon::getByPortId(const int port_id){

	//Mutual exclusion
	ReadScopedLock rlock(rwlock);

	if (port_id != 0) {
		if(port_id_rib_map.find(port_id) == port_id_rib_map.end())
			return NULL;

		return port_id_rib_map[port_id];
	}

	//TODO: improve this. If port-id is 0, it is an operation from the MA
	std::map<rib_handle_t, RIB*>::iterator it;
	for (it = handle_rib_map.begin(); it != handle_rib_map.end(); ++it)
		return it->second;

	return NULL;
}

///
void RIBDaemon::deassociateRIBfromAE(const rib_handle_t& handle,
						const std::string& ae_name){
	__ae_version_key_t key;
	std::map<__ae_version_key_t, RIB*>::iterator it;

	//Mutual exclusion
	WriteScopedLock wlock(rwlock);

	//FIXME: what if the rib is destroyed...
	//Retreive the RIB
	RIB* rib = getRIB(handle);

	if(rib == NULL){
		LOG_ERR("Could not find RIB for handle %" PRId64 ". Already deleted?",
								handle);
		throw eRIBNotFound();
	}

	//Prepare the key
	key.first = ae_name;
	key.second = rib->get_version().version_;

	it = aeversion_rib_map.find(key);

	//Check first if registration exists
	if(it == aeversion_rib_map.end()){
		LOG_ERR("Cannot deassociate RIB '%" PRId64 "' (version: '%" PRId64 "') from AE '%s' because it is not associated!",
						handle,
						rib->get_version().version_,
						ae_name.c_str());
		throw eRIBNotAssociated();
	}

	//And that is the RIB pointed by handle
	if(it->second != rib){
		LOG_ERR("Cannot deassociate RIB '%" PRId64 "' (version: '%" PRId64 "') from AE '%s' because it is not associated!",
						handle,
						rib->get_version().version_,
						ae_name.c_str());
		throw eRIBNotAssociated();
	}

	aeversion_rib_map.erase(key);
}

void RIBDaemon::store_connection(const cdap_rib::con_handle_t& con){

	__ae_version_key_t key;
	std::map<__ae_version_key_t, RIB*>::const_iterator it;
	const uint64_t ver = con.version_.version_;
	const std::string& ae = con.src_.ae_name_;
	const int port_id = con.port_id;

	//Prepare the key
	key.first = ae;
	key.second = ver;

	if((it = aeversion_rib_map.find(key)) == aeversion_rib_map.end()){
		LOG_ERR("No RIB version %" PRIu64 " registered for AE %s!",
								ver,
								ae.c_str());
		throw eRIBNotFound();
	}

	//FIXME: what if the rib is destroyed...
	RIB* rib = it->second;

	//Mutual exclusion
	WriteScopedLock wlock(rwlock);

	//Store relation RIB <-> port_id
	if(port_id_rib_map.find(port_id) != port_id_rib_map.end()){
		LOG_ERR("Overwritting previous connection for RIB version: %" PRIu64 ", AE: %s and port id: %d!",
								ver,
								ae.c_str(),
								port_id);
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
	const int port_id = con.port_id;


	//FIXME: what if the rib is destroyed...
	RIB* rib = getByPortId(port_id);

	if(!rib){
		LOG_ERR("Could not remove connection for port id: %d!",
								port_id);
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

void RIBDaemon::remote_open_connection_result(cdap_rib::con_handle_t &con,
					      const cdap::CDAPMessage& message)
{
	// FIXME remove invoke_id
	app_con_callback_->connectResult(message, con);

	if (message.result_ == 0) {
		try {
			store_connection(con);
		} catch (Exception &e) {
			LOG_WARN("Problems storing connection: %s",
				 e.what());
		}
	}
}

void RIBDaemon::open_connection(cdap_rib::con_handle_t &con,
				const cdap::CDAPMessage& message)
{
	// FIXME add result
	cdap_rib::result_info res;
	app_con_callback_->connect(message, con);

	//The connect was successful store
	try {
		store_connection(con);
	} catch (Exception & e) {
		LOG_WARN("Problems storing connection: %s",
			 e.what());
	}
}

void RIBDaemon::remote_close_connection_result(const cdap_rib::con_handle_t &con,
					       const cdap_rib::result_info &res)
{
	app_con_callback_->releaseResult(res, con);

	try {
		remove_connection(con);
		cdap_provider->get_session_manager()->removeCDAPSession(con.port_id);
	} catch (Exception & e) {
		LOG_WARN("Problems removing connection: %s",
			 e.what());
	}
}

void RIBDaemon::close_connection(const cdap_rib::con_handle_t &con,
				 const cdap_rib::flags_t &flags,
				 const int invoke_id)
{
	cdap_rib::result_info res;
	app_con_callback_->release(invoke_id, con);

	if (invoke_id == 0) {
		try {
			remove_connection(con);
			cdap_provider->get_session_manager()->removeCDAPSession(con.port_id);
		} catch (Exception & e) {
			LOG_WARN("Problems removing connection: %s",
				 e.what());
		}
	}
}

void RIBDaemon::process_authentication_message(const cdap::CDAPMessage& message,
				    	       const cdap_rib::con_handle_t &con)
{
	app_con_callback_->process_authentication_message(message, con);
}

// Object management

int64_t RIBDaemon::addObjRIB(const rib_handle_t& handle,
			     const std::string& fqn,
			     RIBObj** obj){
	if(obj == NULL)
		throw Exception();

	//Mutual exclusion
	ReadScopedLock rlock(rwlock);
	//Retreive the RIB
	RIB* rib = getRIB(handle);

	if(rib == NULL){
		LOG_ERR("Could not add object(%p) to rib('%" PRId64 "'). RIB does not exist",
								*obj,
								handle);
		throw eRIBNotFound();
	}

	return rib->add_obj(fqn, obj);
}

int64_t RIBDaemon::getObjInstId(const rib_handle_t& handle,
				const std::string& fqn,
				const std::string& class_){
	int64_t id;

	//Mutual exclusion
	ReadScopedLock rlock(rwlock);

	//Retreive the RIB
	RIB* rib = getRIB(handle);

	if(rib == NULL){
		LOG_ERR("Could not recover instance id for object '%s'. RIB ('%" PRId64 "') does not exist",
								class_.c_str(),
								handle);
		throw eRIBNotFound();
	}

	id = rib->get_obj_inst_id(fqn);

	if(id < 0)
		throw eObjDoesNotExist();

	if(class_ != ""){
		std::string obj_c = rib->get_obj_class(id);
		if(obj_c != class_)
			throw eObjClassMismatch();
	}

	return id;
}

std::string RIBDaemon::getObjFqn(const rib_handle_t& handle,
				const int64_t inst_id,
				const std::string& class_){

	//Mutual exclusion
	ReadScopedLock rlock(rwlock);

	//Retreive the RIB
	RIB* rib = getRIB(handle);

	if(rib == NULL){
		LOG_ERR("Could not recover instance id for object '%" PRId64 "'. RIB ('%" PRId64 "') does not exist",
								inst_id,
								handle);
		throw eRIBNotFound();
	}

	std::string fqn = rib->get_obj_fqn(inst_id);

	if(fqn == "")
		throw eObjDoesNotExist();

	if(class_ != ""){
		std::string obj_c = rib->get_obj_class(inst_id);
		if(obj_c != class_)
			throw eObjClassMismatch();
	}
	return fqn;
}

std::string RIBDaemon::getObjParentFqn(const rib_handle_t& handle,
						const std::string& fqn){
	//Mutual exclusion
	ReadScopedLock rlock(rwlock);

	//Retreive the RIB
	RIB* rib = getRIB(handle);

	if(rib == NULL){
		LOG_ERR("Could not recover instance id for object '%s'. RIB ('%" PRId64 "') does not exist",
								fqn.c_str(),
								handle);
		throw eRIBNotFound();
	}

	std::string parent_fqn = rib->get_parent_fqn(fqn);

	return parent_fqn;
}


std::string RIBDaemon::getObjClass(const rib_handle_t& handle,
				const int64_t inst_id){
	//Mutual exclusion
	ReadScopedLock rlock(rwlock);

	//Retreive the RIB
	RIB* rib = getRIB(handle);

	if(rib == NULL){
		LOG_ERR("Could not recover instance id for object '%" PRId64 "'. RIB ('%" PRId64 "') does not exist",
								inst_id,
								handle);
		throw eRIBNotFound();
	}

	std::string class_ = rib->get_obj_class(inst_id);

	if(class_ == "")
		throw eObjDoesNotExist();

	return class_;
}

void RIBDaemon::removeObjRIB(const rib_handle_t& handle,
			     const int64_t inst_id,
			     bool force)
{
	//Mutual exclusion
	ReadScopedLock rlock(rwlock);

	//Retreive the RIB
	RIB* rib = getRIB(handle);

	if(rib == NULL){
		LOG_ERR("Could not recover instance id for object '%" PRId64 "'. RIB ('%" PRId64 "') does not exist",
								inst_id,
								handle);
		throw eRIBNotFound();
	}

	rib->remove_obj(inst_id, force);
}

std::list<RIBObjectData> RIBDaemon::get_rib_objects_data(
		const rib_handle_t& handle,
		const std::string& class_,
		const std::string& name)
{
	//Mutual exclusion
	ReadScopedLock rlock(rwlock);

	//Retreive the RIB
	RIB* rib = getRIB(handle);

	if(rib == NULL){
		LOG_ERR("RIB ('%" PRId64 "') does not exist", handle);
		throw eRIBNotFound();
	}

	return rib->get_all_rib_objects_data(class_, name);
}

int RIBDaemon::set_security_manager(ApplicationEntity * sec_man)
{
	if (security_m) {
		LOG_ERR("Security manager already set, cannot set it twice");
		return -1;
	}

	security_m = dynamic_cast<ISecurityManager *>(sec_man);
	if (!security_m) {
		LOG_ERR("Passed pointer to wrong type of AE, it is not Security Manager");
		return -1;
	}

	//Retrieve all the RIB instances and set the security manager
	for(std::map<rib_handle_t, RIB*>::iterator rib_it = handle_rib_map.begin();
			rib_it != handle_rib_map.end(); ++rib_it) {
		rib_it->second->set_security_manager(security_m);
	}
	return 0;
}

int RIBDaemon::remote_operation(const cdap_rib::con_handle_t& con,
                                cdap::CDAPMessage::Opcode opcode,
                                const cdap_rib::obj_info_t &obj,
                                const cdap_rib::flags_t &flags,
                                const cdap_rib::filt_info_t &filt,
                                RIBOpsRespHandler * resp_handler)
{
	cdap_rib::auth_policy_t auth;
	int result = 0;
	int invoke_id = 0;

	// If the operation expects a response, store response handler in table
	if (resp_handler) {
		invoke_id = cdap_provider->get_session_manager()->
				get_invoke_id_manager()->newInvokeId(true);
		response_handlers.put(invoke_id, resp_handler);
	}

	// If there is a security manager and application connection is in place,
	// get access control credential for this operation (if any)
	if (security_m &&
			!cdap_provider->get_session_manager()->session_in_await_con_state(con.port_id)) {
		ISecurityManagerPs *smps = dynamic_cast<rina::ISecurityManagerPs *>(security_m->ps);
		assert(smps);
		if (smps->getAccessControlCreds(auth, con) != 0) {
			LOG_ERR("Error retrieving access control credentials before invoking remote operation");
			return -1;
		}
	}

	try {
		switch(opcode) {
		case cdap::CDAPMessage::M_CREATE:
			result =  cdap_provider->remote_create(con,
							       obj,
							       flags,
							       filt,
							       auth,
							       invoke_id);
			break;
		case cdap::CDAPMessage::M_DELETE:
			result =  cdap_provider->remote_delete(con,
							       obj,
							       flags,
							       filt,
							       auth,
							       invoke_id);
			break;
		case cdap::CDAPMessage::M_READ:
			result =  cdap_provider->remote_read(con,
							     obj,
							     flags,
							     filt,
							     auth,
							     invoke_id);
			break;
		case cdap::CDAPMessage::M_WRITE:
			result =  cdap_provider->remote_write(con,
							      obj,
							      flags,
							      filt,
							      auth,
							      invoke_id);
			break;
		case cdap::CDAPMessage::M_CANCELREAD:
			result =  cdap_provider->remote_cancel_read(con,
							       	    flags,
								    auth,
							            invoke_id);
			break;
		case cdap::CDAPMessage::M_START:
			result =  cdap_provider->remote_start(con,
							      obj,
							      flags,
							      filt,
							      auth,
							      invoke_id);
			break;
		case cdap::CDAPMessage::M_STOP:
			result =  cdap_provider->remote_stop(con,
							     obj,
							     flags,
							     filt,
							     auth,
							     invoke_id);
			break;
		default:
			break;
		}
	} catch (Exception &e) {
		if (resp_handler)
			response_handlers.erase(invoke_id);
		throw e;
	}

	return result;
}

void RIBDaemon::destroy_cdap_connection(int port_id)
{
	try {
		remove_connection(cdap_provider->get_session_manager()->get_con_handle(port_id));
		cdap_provider->get_session_manager()->removeCDAPSession(port_id);
	} catch (Exception & e) {
		LOG_WARN("Problems destroying CDAP session: %s",
			 e.what());
	}
}

//
// Callbacks from events coming from the CDAP provider
//

//Connection events

void RIBDaemon::remote_create_result(const cdap_rib::con_handle_t &con,
				     const cdap_rib::obj_info_t &obj,
				     const cdap_rib::res_info_t &res,
				     const int invoke_id)
{
	RIBOpsRespHandler * handler;

	handler = check_rib_and_get_response_handler(con,
						     invoke_id,
						     true);
	if (handler) {
		try {
			handler->remoteCreateResult(con,
						    obj,
						    res);
		} catch (Exception &e) {
			LOG_ERR("Unable to process the response");
		}
	}
}

void RIBDaemon::remote_delete_result(const cdap_rib::con_handle_t &con,
				     const cdap_rib::obj_info_t &obj,
				     const cdap_rib::res_info_t &res,
				     const int invoke_id)
{
	RIBOpsRespHandler * handler;

	handler = check_rib_and_get_response_handler(con,
						     invoke_id,
						     true);
	if (handler) {
		try {
			handler->remoteDeleteResult(con,
						    obj,
						    res);
		} catch (Exception &e) {
			LOG_ERR("Unable to process the response");
		}
	}
}

void RIBDaemon::remote_read_result(const cdap_rib::con_handle_t &con,
				   const cdap_rib::obj_info_t &obj,
				   const cdap_rib::res_info_t &res,
				   const cdap_rib::flags_t &flags,
				   const int invoke_id)
{
	RIBOpsRespHandler * handler;
	bool remove = true;

	if (flags.flags_ == cdap_rib::flags_t::F_RD_INCOMPLETE)
		remove = false;

	handler = check_rib_and_get_response_handler(con,
						     invoke_id,
						     remove);
	if (handler) {
		try {
			handler->remoteReadResult(con,
						  obj,
						  res,
						  flags);
		} catch (Exception &e) {
			LOG_ERR("Unable to process the response");
		}
	}
}

void RIBDaemon::remote_cancel_read_result(const cdap_rib::con_handle_t &con,
					  const cdap_rib::res_info_t &res,
					  const int invoke_id)
{
	RIBOpsRespHandler * handler;

	handler = check_rib_and_get_response_handler(con,
						     invoke_id,
						     true);
	if (handler) {
		try {
			handler->remoteCancelReadResult(con,
						    	res);
		} catch (Exception &e) {
			LOG_ERR("Unable to process the response");
		}
	}
}

void RIBDaemon::remote_write_result(const cdap_rib::con_handle_t &con,
				    const cdap_rib::obj_info_t &obj,
				    const cdap_rib::res_info_t &res,
				    const int invoke_id)
{
	RIBOpsRespHandler * handler;

	handler = check_rib_and_get_response_handler(con,
						     invoke_id,
						     true);
	if (handler) {
		try {
			handler->remoteWriteResult(con,
						   obj,
						   res);
		} catch (Exception &e) {
			LOG_ERR("Unable to process the response");
		}
	}
}

void RIBDaemon::remote_start_result(const cdap_rib::con_handle_t &con,
				    const cdap_rib::obj_info_t &obj,
				    const cdap_rib::res_info_t &res,
				    const int invoke_id)
{
	RIBOpsRespHandler * handler;

	handler = check_rib_and_get_response_handler(con,
						     invoke_id,
						     true);
	if (handler) {
		try {
			handler->remoteStartResult(con,
						   obj,
						   res);
		} catch (Exception &e) {
			LOG_ERR("Unable to process the response");
		}
	}
}

void RIBDaemon::remote_stop_result(const cdap_rib::con_handle_t &con,
				   const cdap_rib::obj_info_t &obj,
				   const cdap_rib::res_info_t &res,
				   const int invoke_id)
{
	RIBOpsRespHandler * handler;

	handler = check_rib_and_get_response_handler(con,
						     invoke_id,
						     true);
	if (handler) {
		try {
			handler->remoteStopResult(con,
						  obj,
						  res);
		} catch (Exception &e) {
			LOG_ERR("Unable to process the response");
		}
	}
}

//CDAP object operations
void RIBDaemon::create_request(const cdap_rib::con_handle_t &con,
			       const cdap_rib::obj_info_t &obj,
			       const cdap_rib::filt_info_t &filt,
			       const cdap_rib::auth_policy_t& auth,
			       const int invoke_id)
{
	//TODO this is not safe if RIB instances can be deleted
	RIB* rib = getByPortId(con.port_id);

	if(!rib){
		cdap_rib::res_info_t res;
		cdap_rib::flags_t flags;
		res.code_ = cdap_rib::CDAP_ERROR;
		try {
			cdap_provider->send_create_result(con,
							  obj,
							  flags,
							  res,
							  invoke_id);
		} catch (Exception &e) {
			LOG_ERR("Unable to send the response");
		}
		return;
	}

	//Invoke
	rib->create_request(con, obj, filt, auth, invoke_id);
}

void RIBDaemon::delete_request(const cdap_rib::con_handle_t &con,
			       const cdap_rib::obj_info_t &obj,
			       const cdap_rib::filt_info_t &filt,
			       const cdap_rib::auth_policy_t& auth,
			       const int invoke_id)
{
	//TODO this is not safe if RIB instances can be deleted
	RIB* rib = getByPortId(con.port_id);

	if(!rib){
		cdap_rib::res_info_t res;
		cdap_rib::flags_t flags;
		res.code_ = cdap_rib::CDAP_ERROR;
		try {
			cdap_provider->send_delete_result(con,
							  obj,
							  flags,
							  res,
							  invoke_id);
		} catch (Exception &e) {
			LOG_ERR("Unable to send the response");
		}
		return;
	}

	//Invoke
	rib->delete_request(con, obj, filt, auth, invoke_id);
}

void RIBDaemon::read_request(const cdap_rib::con_handle_t &con,
			     const cdap_rib::obj_info_t &obj,
			     const cdap_rib::filt_info_t &filt,
			     const cdap_rib::flags_t &flags,
			     const cdap_rib::auth_policy_t& auth,
			     const int invoke_id)
{
	//TODO this is not safe if RIB instances can be deleted
	RIB* rib = getByPortId(con.port_id);

	if(!rib){
		cdap_rib::res_info_t res;
		cdap_rib::flags_t flags;
		res.code_ = cdap_rib::CDAP_ERROR;
		try {
			cdap_provider->send_read_result(con,
							obj,
							flags,
							res,
							invoke_id);
		} catch (Exception &e) {
			LOG_ERR("Unable to send the response");
		}
		return;
	}

	//Invoke
	rib->read_request(con, obj, filt, flags, auth, invoke_id);
}
void RIBDaemon::cancel_read_request(const cdap_rib::con_handle_t &con,
				    const cdap_rib::obj_info_t &obj,
				    const cdap_rib::filt_info_t &filt,
				    const cdap_rib::auth_policy_t& auth,
				    const int invoke_id)
{
	//TODO this is not safe if RIB instances can be deleted
	RIB* rib = getByPortId(con.port_id);

	if(!rib){
		cdap_rib::res_info_t res;
		cdap_rib::flags_t flags;
		res.code_ = cdap_rib::CDAP_ERROR;
		try {
			cdap_provider->send_cancel_read_result(con,
							       flags,
							       res,
							       invoke_id);
		} catch (Exception &e) {
			LOG_ERR("Unable to send the response");
		}
		return;
	}

	//Invoke
	rib->cancel_read_request(con, obj, filt, auth, invoke_id);
}
void RIBDaemon::write_request(const cdap_rib::con_handle_t &con,
			      const cdap_rib::obj_info_t &obj,
			      const cdap_rib::filt_info_t &filt,
			      const cdap_rib::auth_policy_t& auth,
			      const int invoke_id)
{
	//TODO this is not safe if RIB instances can be deleted
	RIB* rib = getByPortId(con.port_id);

	if(!rib){
		cdap_rib::res_info_t res;
		cdap_rib::flags_t flags;
		res.code_ = cdap_rib::CDAP_ERROR;
		try {
			cdap_provider->send_write_result(con,
							 flags,
							 res,
							 invoke_id);
		} catch (Exception &e) {
			LOG_ERR("Unable to send the response");
		}
		return;
	}

	//Invoke
	rib->write_request(con, obj, filt, auth, invoke_id);
}
void RIBDaemon::start_request(const cdap_rib::con_handle_t &con,
			      const cdap_rib::obj_info_t &obj,
			      const cdap_rib::filt_info_t &filt,
			      const cdap_rib::auth_policy_t& auth,
			      const int invoke_id)
{
	//TODO this is not safe if RIB instances can be deleted
	RIB* rib = getByPortId(con.port_id);

	if(!rib){
		cdap_rib::res_info_t res;
		cdap_rib::flags_t flags;
		res.code_ = cdap_rib::CDAP_ERROR;
		try {
			cdap_provider->send_start_result(con,
							 obj,
							 flags,
							 res,
							 invoke_id);
		} catch (Exception &e) {
			LOG_ERR("Unable to send the response");
		}
		return;
	}

	//Invoke
	rib->start_request(con, obj, filt, auth, invoke_id);
}
void RIBDaemon::stop_request(const cdap_rib::con_handle_t &con,
			     const cdap_rib::obj_info_t &obj,
			     const cdap_rib::filt_info_t &filt,
			     const cdap_rib::auth_policy_t& auth,
			     const int invoke_id)
{
	//TODO this is not safe if RIB instances can be deleted
	RIB* rib = getByPortId(con.port_id);

	if(!rib){
		cdap_rib::res_info_t res;
		cdap_rib::flags_t flags;
		res.code_ = cdap_rib::CDAP_ERROR;
		try {
			cdap_provider->send_stop_result(con,
							flags,
							res,
							invoke_id);
		} catch (Exception &e) {
			LOG_ERR("Unable to send the response");
		}
		return;
	}

	//Invoke
	rib->stop_request(con, obj, filt, auth, invoke_id);
}

//RIBObj/RIBObj_
RIBObjectData RIBObj::get_object_data()
{
	RIBObjectData result;
	result.name_ = fqn;
	result.class_ = class_name;
	result.displayable_value_ = get_displayable_value();

	return result;
}

void RIBObj::create(const cdap_rib::con_handle_t &con,
		    const std::string& fqn,
		    const std::string& class_,
		    const cdap_rib::filt_info_t &filt,
		    const int invoke_id,
		    const ser_obj_t &obj_req,
		    ser_obj_t &obj_reply,
		    cdap_rib::res_info_t& res)
{
	operation_not_supported(res);
}

bool RIBObj::delete_(const cdap_rib::con_handle_t &con,
		     const std::string& fqn,
		     const std::string& class_,
		     const cdap_rib::filt_info_t &filt,
		     const int invoke_id,
		     cdap_rib::res_info_t& res)
{
	operation_not_supported(res);
	return false;
}

// FIXME remove name, it is not needed
void RIBObj::read(const cdap_rib::con_handle_t &con,
		  const std::string& fqn,
		  const std::string& class_,
		  const cdap_rib::filt_info_t &filt,
		  const int invoke_id,
		  cdap_rib::obj_info_t &obj_reply,
		  cdap_rib::res_info_t& res)
{
	operation_not_supported(res);
}

void RIBObj::cancelRead(const cdap_rib::con_handle_t &con,
			const std::string& fqn,
			const std::string& class_,
			const cdap_rib::filt_info_t &filt,
			const int invoke_id,
			cdap_rib::res_info_t& res)
{
	operation_not_supported(res);
}

void RIBObj::write(const cdap_rib::con_handle_t &con,
		   const std::string& fqn,
		   const std::string& class_,
		   const cdap_rib::filt_info_t &filt,
		   const int invoke_id,
		   const ser_obj_t &obj_req,
		   ser_obj_t &obj_reply,
		   cdap_rib::res_info_t& res)
{
	operation_not_supported(res);
}

void RIBObj::start(const cdap_rib::con_handle_t &con,
		   const std::string& fqn,
		   const std::string& class_,
		   const cdap_rib::filt_info_t &filt,
		   const int invoke_id,
		   const ser_obj_t &obj_req,
		   ser_obj_t &obj_reply,
		   cdap_rib::res_info_t& res)
{
	operation_not_supported(res);
}

void RIBObj::stop(const cdap_rib::con_handle_t &con,
		  const std::string& fqn,
		  const std::string& class_,
		  const cdap_rib::filt_info_t &filt,
		  const int invoke_id,
		  const ser_obj_t &obj_req,
		  ser_obj_t &obj_reply,
		  cdap_rib::res_info_t& res)
{
	operation_not_supported(res);
}

void RIBObj::operation_not_supported(cdap_rib::res_info_t& res) {
	LOG_INFO("Operation over a RIB object not supported");
	res.code_ = cdap_rib::CDAP_OP_NOT_SUPPORTED;
}

// DelegationObj

DelegationObj::DelegationObj(const std::string &class_name):
		RIBObj(class_name) {
	delegates = true;
	finished = true;
	last = false;
}

DelegationObj::~DelegationObj(void) throw ()
{}

void DelegationObj::forwarded_object_response(int port, int invoke_id,
		rina::cdap::cdap_m_t *msg)
{
        bool signal = false;
        rina::cdap_rib::con_handle_t con;
        con.port_id = port;
        msg->invoke_id_ = invoke_id;
        msg->obj_name_ = fqn  + msg->obj_name_;
        if (msg->flags_ != rina::cdap_rib::flags_t::F_RD_INCOMPLETE)
        {
                if (!last)
                        msg->flags_=rina::cdap_rib::flags_t::F_RD_INCOMPLETE;
                signal = true;
        }
        rina::cdap::getProvider()->send_cdap_result(con,  msg);
        if (signal)
                signal_finished();
}

void DelegationObj::is_finished()
{
	lock();
	if (!finished)
		doWait();
	unlock();
}
void DelegationObj::signal_finished()
{
	lock();
	finished = true;
	signal();
	unlock();
}

void DelegationObj::activate_delegation()
{
	lock();
	finished = false;
	unlock();
}

//Single RIBDaemon instance
static RIBDaemon* ribd = NULL;

//
// RIBDaemon (public) interface (proxy)
//

//Constructor (private)
RIBDaemonProxy::RIBDaemonProxy(RIBDaemon* ribd_)
{
	ribd = ribd_;
};

//
// Local RIB
//

void RIBDaemonProxy::createSchema(const cdap_rib::vers_info_t& v,
								const char s){
	ribd->createSchema(v, s);
}

std::list<cdap_rib::vers_info_t> RIBDaemonProxy::listVersions(void){
	return ribd->listVersions();
}

void RIBDaemonProxy::addCreateCallbackSchema(const cdap_rib::vers_info_t& version,
					     const std::string& class_,
					     const std::string& fqn_,
					     create_cb_t cb)
{
	return ribd->addCreateCallbackSchema(version,
					     class_,
					     fqn_,
					     cb);
}

void RIBDaemonProxy::destroySchema(const cdap_rib::vers_info_t& v)
{
	ribd->destroySchema(v);
}

rib_handle_t RIBDaemonProxy::createRIB(const cdap_rib::vers_info_t& v,
		        	       const std::string& base_file_path){
	return ribd->createRIB(v, base_file_path);
}

void RIBDaemonProxy::destroyRIB(const rib_handle_t& h){
	ribd->destroyRIB(h);
}

void RIBDaemonProxy::associateRIBtoAE(const rib_handle_t& h,
				      const std::string& ae)
{
	ribd->associateRIBtoAE(h, ae);
}

void RIBDaemonProxy::deassociateRIBfromAE(const rib_handle_t& h,
					  const std::string& ae)
{
	ribd->deassociateRIBfromAE(h, ae);
}

rib_handle_t RIBDaemonProxy::get(const cdap_rib::vers_info_t& v,
				 const std::string& ae)
{
	return ribd->get(v, ae);
}

//RIB object mamangement

int64_t RIBDaemonProxy::__addObjRIB(const rib_handle_t& h,
				    const std::string& fqn,
				    RIBObj** o){
	return ribd->addObjRIB(h, fqn, o);
}

int64_t RIBDaemonProxy::getObjInstId(const rib_handle_t& h,
				const std::string& fqn,
				const std::string& c){
	return ribd->getObjInstId(h, fqn, c);
}

std::string RIBDaemonProxy::getObjParentFqn(const rib_handle_t& h,
						const std::string& fqn){
	return ribd->getObjParentFqn(h, fqn);
}

std::string RIBDaemonProxy::getObjFqn(const rib_handle_t& h,
				const int64_t id,
				const std::string& c){
	return ribd->getObjFqn(h, id, c);
}

std::string RIBDaemonProxy::getObjClass(const rib_handle_t& h,
				const int64_t id){
	return ribd->getObjClass(h, id);
}

void RIBDaemonProxy::removeObjRIB(const rib_handle_t& h,
				  const int64_t id,
				  bool force)
{
	return ribd->removeObjRIB(h, id, force);
}

void RIBDaemonProxy::removeObjRIB(const rib_handle_t& handle,
				  const std::string fqdn,
				  bool force)
{
	removeObjRIB(handle, getObjInstId(handle, fqdn), force);
}

bool RIBDaemonProxy::containsObj(const rib_handle_t& handle,
		 	 	 const std::string fqdn)
{
	try {
		getObjInstId(handle, fqdn);
		return true;
	} catch (Exception &e) {
		return false;
	}
}

std::list<RIBObjectData> RIBDaemonProxy::get_rib_objects_data(
		const rib_handle_t& handle,
		const std::string& class_,
		const std::string& name)
{
	return ribd->get_rib_objects_data(handle, class_, name);
}

int RIBDaemonProxy::set_security_manager(ApplicationEntity * sec_man)
{
	return ribd->set_security_manager(sec_man);
}

//
// Client
//

// Establish a CDAP connection to a remote RIB
cdap_rib::con_handle_t RIBDaemonProxy::remote_open_connection(const cdap_rib::vers_info_t &ver,
							      const cdap_rib::ep_info_t &src,
							      const cdap_rib::ep_info_t &dest,
							      const cdap_rib::auth_policy &auth,
							      int port_id)
{
	cdap_rib::con_handle_t handle =
		ribd->cdap_provider->remote_open_connection(ver,
							    src,
							    dest,
							    auth,
							    port_id);
	//TODO store?
	return handle;
}

// Close a CDAP connection to a remote RIB
int RIBDaemonProxy::remote_close_connection(unsigned int port,
					    bool need_reply)
{
	int res = ribd->cdap_provider->remote_close_connection(port,
							       need_reply);

	if (!need_reply) {
		ribd->destroy_cdap_connection(port);
	}

	return res;
}

// Perform a create operation over an object of the remote RIB
int RIBDaemonProxy::remote_create(const cdap_rib::con_handle_t& con,
				  const cdap_rib::obj_info_t &obj,
				  const cdap_rib::flags_t &flags,
				  const cdap_rib::filt_info_t &filt,
				  RIBOpsRespHandler * resp_handler)
{
	return ribd->remote_operation(con,
				      cdap::CDAPMessage::M_CREATE,
				      obj,
				      flags,
				      filt,
				      resp_handler);
}

// Perform a delete operation over an object of the remote RIB
int RIBDaemonProxy::remote_delete(const cdap_rib::con_handle_t& con,
			  	  const cdap_rib::obj_info_t &obj,
			  	  const cdap_rib::flags_t &flags,
			  	  const cdap_rib::filt_info_t &filt,
			  	  RIBOpsRespHandler * resp_handler)
{
	return ribd->remote_operation(con,
				      cdap::CDAPMessage::M_DELETE,
				      obj,
				      flags,
				      filt,
				      resp_handler);
}

// Perform a read operation over an object of the remote RIB
int RIBDaemonProxy::remote_read(const cdap_rib::con_handle_t& con,
				const cdap_rib::obj_info_t &obj,
				const cdap_rib::flags_t &flags,
				const cdap_rib::filt_info_t &filt,
				RIBOpsRespHandler * resp_handler)
{
	return ribd->remote_operation(con,
				      cdap::CDAPMessage::M_READ,
				      obj,
				      flags,
				      filt,
				      resp_handler);
}

// Perform a cancel read operation over an object of the remote RIB
int RIBDaemonProxy::remote_cancel_read(const cdap_rib::con_handle_t& con,
				       const cdap_rib::flags_t &flags,
				       const int invoke_id,
				       RIBOpsRespHandler * resp_handler)
{
	cdap_rib::obj_info_t obj;
	cdap_rib::filt_info_t filt;

	return ribd->remote_operation(con,
                              cdap::CDAPMessage::M_CANCELREAD,
                              obj,
                              flags,
                              filt,
                              resp_handler);
}

// Perform a write operation over an object of the remote RIB
int RIBDaemonProxy::remote_write(const cdap_rib::con_handle_t& con,
			 	 const cdap_rib::obj_info_t &obj,
			 	 const cdap_rib::flags_t &flags,
			 	 const cdap_rib::filt_info_t &filt,
			 	 RIBOpsRespHandler * resp_handler)
{
	return ribd->remote_operation(con,
                              cdap::CDAPMessage::M_WRITE,
                              obj,
                              flags,
                              filt,
                              resp_handler);
}

// Perform a start operation over an object of the remote RIB
int RIBDaemonProxy::remote_start(const cdap_rib::con_handle_t& con,
			 	 const cdap_rib::obj_info_t &obj,
			 	 const cdap_rib::flags_t &flags,
			 	 const cdap_rib::filt_info_t &filt,
			 	 RIBOpsRespHandler * resp_handler)
{
	return ribd->remote_operation(con,
                              cdap::CDAPMessage::M_START,
                              obj,
                              flags,
                              filt,
                              resp_handler);
}

// Perform a stop operation over an object of the remote RIB
int RIBDaemonProxy::remote_stop(const cdap_rib::con_handle_t& con,
				const cdap_rib::obj_info_t &obj,
				const cdap_rib::flags_t &flags,
				const cdap_rib::filt_info_t &filt,
				RIBOpsRespHandler * resp_handler)
{
	return ribd->remote_operation(con,
                                cdap::CDAPMessage::M_STOP,
                                obj,
                                flags,
                                filt,
                                resp_handler);
}

// Class RIBDaemonAE
void RIBDaemonAE::set_application_process(ApplicationProcess * ap)
{
	(void) ap;
}

int RIBDaemonAE::set_security_manager(ApplicationEntity * sec_man)
{
	return getProxy()->set_security_manager(sec_man);
}

//
// RIB library routines
//
void init(cacep::AppConHandlerInterface *app_con_callback,
	  cdap_rib::cdap_params params)
{

	//Callbacks
	if(ribd){
		LOG_ERR("Double call to rina::rib::init()");
		throw Exception("Double call to rina::rib::init()");
	}

	//Initialize intializes the RIB daemon
	ribd = new RIBDaemon(app_con_callback, params);
}

RIBDaemonProxy* RIBDaemonProxyFactory(){
	if(!ribd){
		LOG_ERR("RIB library not initialized! Call rib::init() method first");
		throw Exception("Double call to rina::rib::init()");

	}
	return new RIBDaemonProxy(ribd);
}

void __set_cdap_provider(cdap::CDAPProviderInterface* p){
	ribd->__set_cdap_provider(p);
}
cdap::CDAPCallbackInterface* __get_rib_provider(){
	return ribd;
}

void fini(){
	delete ribd;
}

/* Class RIBOpsRespHandlers */
void RIBOpsRespHandler::remoteCreateResult(const cdap_rib::con_handle_t &con,
					    const cdap_rib::obj_info_t &obj,
					    const cdap_rib::res_info_t &res)
{
	operation_not_supported();
}

void RIBOpsRespHandler::remoteDeleteResult(const cdap_rib::con_handle_t &con,
					   const cdap_rib::obj_info_t &obj,
					   const cdap_rib::res_info_t &res)
{
	operation_not_supported();
}

void RIBOpsRespHandler::remoteReadResult(const cdap_rib::con_handle_t &con,
			      	      	  const cdap_rib::obj_info_t &obj,
			      	      	  const cdap_rib::res_info_t &res,
					  const cdap_rib::flags_t & flags)
{
	operation_not_supported();
}

void RIBOpsRespHandler::remoteCancelReadResult(const cdap_rib::con_handle_t &con,
				    	        const cdap_rib::res_info_t &res)
{
	operation_not_supported();
}

void RIBOpsRespHandler::remoteWriteResult(const cdap_rib::con_handle_t &con,
			       	       	   const cdap_rib::obj_info_t &obj,
			       	       	   const cdap_rib::res_info_t &res)
{
	operation_not_supported();
}

void RIBOpsRespHandler::remoteStartResult(const cdap_rib::con_handle_t &con,
			       	           const cdap_rib::obj_info_t &obj,
			       	           const cdap_rib::res_info_t &res)
{
	operation_not_supported();
}

void RIBOpsRespHandler::remoteStopResult(const cdap_rib::con_handle_t &con,
			      	          const cdap_rib::obj_info_t &obj,
			      	          const cdap_rib::res_info_t &res)
{
	operation_not_supported();
}

void RIBOpsRespHandler::operation_not_supported() {
	LOG_WARN("Operation not supported");
}

/* Class RIBObjectData*/
RIBObjectData::RIBObjectData(){
        instance_ = 0;
}

RIBObjectData::RIBObjectData(
                std::string clazz, std::string name,
                long long instance){
        this->class_ = clazz;
        this->name_ = name;
        this->instance_ = instance;
}

bool RIBObjectData::operator==(const RIBObjectData &other) const{
        if (class_.compare(other.get_class()) != 0) {
                return false;
        }

        if (name_.compare(other.get_name()) != 0) {
                return false;
        }

        return instance_ == other.get_instance();
}

bool RIBObjectData::operator!=(const RIBObjectData &other) const{
        return !(*this == other);
}

const std::string& RIBObjectData::get_class() const {
        return class_;
}

void RIBObjectData::set_class(const std::string& clazz) {
        class_ = clazz;
}

unsigned long RIBObjectData::get_instance() const {
        return instance_;
}

void RIBObjectData::set_instance(unsigned long instance) {
        instance_ = instance;
}

const std::string& RIBObjectData::get_name() const {
        return name_;
}

void RIBObjectData::set_name(const std::string& name) {
        name_ = name;
}

const std::string& RIBObjectData::get_displayable_value() const {
        return displayable_value_;
}

void RIBObjectData::set_displayable_value(const std::string& displayable_value) {
        displayable_value_ = displayable_value;
}

}  //namespace rib
}  //namespace rina
