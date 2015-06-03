/*
 * RIB API
 *
 *    Bernat Gastón <bernat.gaston@i2cat.net>
 *    Eduard Grasa          <eduard.grasa@i2cat.net>
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
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
#ifndef RIB_PROVIDER_H_
#define RIB_PROVIDER_H_
#include "cdap_rib_structures.h"
#include <string>
#include <inttypes.h>
#include <list>
#include <map>
#include <algorithm>
#include "librina/concurrency.h"
#include "librina/exceptions.h"

namespace rina{
namespace cacep {

// FIXME: this class is only used in enrollment, it must go in a different file that rib
class AppConHandlerInterface {

public:
	virtual ~AppConHandlerInterface(){};

	/// A remote Connect request has been received.
	virtual void connect(int invoke_id, const cdap_rib::con_handle_t &con) = 0;
	/// A remote Connect response has been received.
	virtual void connectResult(const cdap_rib::res_info_t &res,
			const cdap_rib::con_handle_t &con) = 0;
	/// A remote Release request has been received.
	virtual void release(int invoke_id, const cdap_rib::con_handle_t &con) = 0;
	/// A remote Release response has been received.
	virtual void releaseResult(const cdap_rib::res_info_t &res,
			const cdap_rib::con_handle_t &con) = 0;
};

}//namespace cacep

namespace rib {

//fwd decl
class RIBDaemonProxy;
class RIBOpsRespHandlers;

//
// Schema exceptions
//

/// A schema with the same version has been already created
DECLARE_EXCEPTION_SUBCLASS(eSchemaExists);

/// The schema cannot removed because there are RIBs using it
DECLARE_EXCEPTION_SUBCLASS(eSchemaInUse);

/// The schema does not exist
DECLARE_EXCEPTION_SUBCLASS(eSchemaNotFound);

//
// RIB&AE management
//

/// RIB version has been already registered for this AE
DECLARE_EXCEPTION_SUBCLASS(eRIBAlreadyAssociated);

/// Could not find a valid RIB version for this AE
DECLARE_EXCEPTION_SUBCLASS(eRIBNotFound);

/// RIB is being used by an AE
DECLARE_EXCEPTION_SUBCLASS(eRIBInUse);

/// RIB is not registered in this AE
DECLARE_EXCEPTION_SUBCLASS(eRIBNotAssociated);

/// RIB is not registered in this AE
DECLARE_EXCEPTION_SUBCLASS(eNotImplemented);

//
// RIB object management
//

/// Invalid object
DECLARE_EXCEPTION_SUBCLASS(eObjInvalid);

/// Validation error; the operation was rejected by the rules defined in
/// the schema.
DECLARE_EXCEPTION_SUBCLASS(eOpValidation);

/// An object already exists in the same position of the tree
DECLARE_EXCEPTION_SUBCLASS(eObjExists);

/// Parent's object does not exist
DECLARE_EXCEPTION_SUBCLASS(eObjNoParent);

/// The object does not exist in that position of the tree
DECLARE_EXCEPTION_SUBCLASS(eObjDoesNotExist);

/// The object exists but the class name mismatches
DECLARE_EXCEPTION_SUBCLASS(eObjClassMismatch);





///
/// Initialize the RIB library (RIBDaemon)
///
/// This method initializes the state of the RIB library. It does:
///
/// * Initialize internal state of the RIB library (RIBDaemon)
/// * Intiialize the CDAP provider
///
///
void init(cacep::AppConHandlerInterface *app_con_callback,
		RIBOpsRespHandlers* remote_handlers,
		cdap_rib::cdap_params params);

//
// Get a proxy object to interface the RIBDaemon
//
// @ret A proxy object to the RIBDaemon
//
RIBDaemonProxy* RIBDaemonProxyFactory();


///
/// Destroy the RIB library state
///
void fini(void);

//
// RIB operations response handlers
//
class RIBOpsRespHandlers {

public:
	virtual ~RIBOpsRespHandlers(){};

	virtual void remoteCreateResult(const cdap_rib::con_handle_t &con,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::res_info_t &res) = 0;
	virtual void remoteDeleteResult(const cdap_rib::con_handle_t &con,
			const cdap_rib::res_info_t &res) = 0;
	virtual void remoteReadResult(const cdap_rib::con_handle_t &con,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::res_info_t &res) = 0;
	virtual void remoteCancelReadResult(const cdap_rib::con_handle_t &con,
			const cdap_rib::res_info_t &res) = 0;
	virtual void remoteWriteResult(const cdap_rib::con_handle_t &con,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::res_info_t &res) = 0;
	virtual void remoteStartResult(const cdap_rib::con_handle_t &con,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::res_info_t &res) = 0;
	virtual void remoteStopResult(const cdap_rib::con_handle_t &con,
			const cdap_rib::obj_info_t &obj,
			const cdap_rib::res_info_t &res) = 0;
};

class AbstractEncoder {

public:
	virtual ~AbstractEncoder();
	virtual std::string get_type() const = 0;
	bool operator=(const AbstractEncoder &other) const;
	bool operator!=(const AbstractEncoder &other) const;
};

template<class T>
class Encoder: public AbstractEncoder {

public:
	virtual ~Encoder(){}

	/// Converts an object to a byte array, if this object is recognized by the encoder
	/// @param object
	/// @throws exception if the object is not recognized by the encoder
	/// @return
	virtual void encode(const T &obj, cdap_rib::ser_obj_t& serobj) = 0;
	/// Converts a byte array to an object of the type specified by "className"
	/// @param byte[] serializedObject
	/// @param objectClass The type of object to be decoded
	/// @throws exception if the byte array is not an encoded in a way that the
	/// encoder can recognize, or the byte array value doesn't correspond to an
	/// object of the type "className"
	/// @return
	virtual void decode(const cdap_rib::ser_obj_t &serobj,
			T& des_obj) = 0;
};

//fwd decl
template <typename T>
class RIBObj;

///
/// @internal non-template dependent base RIBObj
/// Base RIB Object. API for the create/delete/read/write/start/stop RIB
/// functionality for certain objects (identified by objectNames)
///
class RIBObj_{

public:

	///
	/// Constructor
	///
	/// @param user User-specific context (type associated)
	/// @param fqn Fully qualifed name (
	///
	RIBObj_() : delegates(false){};

	/// Fully qualified name
	const std::string fqn;

	/// Destructor
	virtual ~RIBObj_(){};

protected:
	///
	/// Remote invocations, resulting from CDAP messages
	///

	///
	/// Process a remote create
	///
	/// @param name FQN of the object
	/// @param obj_req Optional serialized object from the request.
	///                Shall only be decoded if size != 0
	/// @param obj_reply Optional serialized object to be returned.
	///                  Shall only be decoded if size != 0
	///                  Initialized to size = 0 by default.
	///
	virtual cdap_rib::res_info_t* create(const std::string& name, const std::string clas,
			const cdap_rib::SerializedObject &obj_req,
			cdap_rib::SerializedObject &obj_reply);
	///
	/// Process a remote delete operation
	///
	/// @param name FQN of the object
	///
	virtual cdap_rib::res_info_t* delete_(const std::string& name);

	///
	///
	/// Process a remote read operation
	///
	/// @param name FQN of the object
	/// @obj_reply Serialized object to be returned.
	///
	virtual cdap_rib::res_info_t* read(const std::string& name,
			cdap_rib::SerializedObject &obj_reply);

	///
	///
	/// Process a cancel remote read operation
	///
	/// @param name FQN of the object
	///
	virtual cdap_rib::res_info_t* cancelRead(const std::string& name);

	///
	///
	/// Process a remote write operation
	///
	/// @param name FQN of the object
	/// @param obj_req Serialized object from the request
	/// @param obj_reply Optional serialized object to be returned.
	///                  Will only be decoded by the RIB library if size != 0.
	///                  Initialized to size = 0 by default.
	///
	virtual cdap_rib::res_info_t* write(const std::string& name,
			const cdap_rib::SerializedObject &obj_req,
			cdap_rib::SerializedObject &obj_reply);

	///
	///
	/// Process a remote read operation
	///
	/// @param name FQN of the object
	/// @param obj_req Optional serialized object from the request.
	///                Shall only be decoded if size != 0
	/// @param obj_reply Optional serialized object to be returned.
	///                  Shall only be decoded if size != 0
	///                  Initialized to size = 0 by default.
	///
	virtual cdap_rib::res_info_t* start(const std::string& name,
			const cdap_rib::SerializedObject &obj_req,
			cdap_rib::SerializedObject &obj_reply);

	///
	///
	/// Process a remote read operation
	///
	/// @param name FQN of the object
	/// @param obj_req Optional serialized object from the request.
	///                Shall only be decoded if size != 0
	/// @param obj_reply Optional serialized object to be returned.
	///                  Shall only be decoded if size != 0
	///                  Initialized to size = 0 by default.
	///
	virtual cdap_rib::res_info_t* stop(const std::string& name,
			const cdap_rib::SerializedObject &obj_req,
			cdap_rib::SerializedObject &obj_reply);

	///
	/// Get the class name.
	///
	/// Method that inheriting classes MUST implementing returing the
	/// class name
	///
	///
	virtual const std::string& get_class() const = 0;

	///
	/// Throw not supported exception
	///
	void operation_not_supported();

	///
	/// Get the encoder
	///
	/// TODO: remove?
	///
	virtual AbstractEncoder* get_encoder() const = 0;

	///Rwlock
	rina::ReadWriteLockable rwlock;

	//Flag used to identify objects which delegates a portion of the tree
	bool delegates;

	//They love each other
	template<typename T> friend class RIBObj;

	//Them too; promiscuous?
	friend class RIB;
};
///
/// Base RIB Object. API for the create/delete/read/write/start/stop RIB
/// functionality for certain objects (identified by objectNames)
///
/// @template T The user-data type. Must support assignation operator
///
template<typename T>
class RIBObj : public RIBObj_{

public:

	///
	/// Constructor
	///
	/// @param user User-specific context (type associated)
	/// @param fqn Fully qualifed name (
	///
	RIBObj(T user_data_){
		user_data = user_data_;
	}


	///
	/// Retrieve user data
	///
	virtual T get_user_data(void){
		//Mutual exclusion
		ReadScopedLock wlock(rwlock);

		return user_data;
	}

	///
	/// Set user data
	///
	virtual void set_user_data(T new_user_data){
		//Mutual exclusion
		WriteScopedLock wlock(rwlock);

		user_data = new_user_data;
	}

	/// Destructor
	virtual ~RIBObj(){};

protected:
	///
	/// User data
	///
	T user_data;
};


///
/// This class is used to capture operations on objects in a part of the tree
/// without having to add explicitely the objects (catch all)
///
class RIBDelegationObj : public RIBObj<void*>{

	/// Constructor
	RIBDelegationObj(void) : RIBObj(NULL) {
		delegates = true;
	};

	//Destructor
	~RIBDelegationObj(void){};
};

///
/// RIB library result codes
///
enum rib_schema_res {

	RIB_SUCCESS,
	/* The RIB schema file extension is unknown */
	RIB_SCHEMA_EXT_ERR = -3,
	/* Error during RIB scheema file parsing */
	RIB_SCHEMA_FORMAT_ERR = -4,
	/* General validation error (unknown) */
	RIB_SCHEMA_VALIDATION_ERR = -5,
	/* Validation error, missing mandatory object */
	RIB_SCHEMA_VAL_MAN_ERR = -6,
	//
	// Misc
	//
	//TODO: Other error codes
};

///
/// RIB Schema Object
///
class RIBSchemaObject {

public:
	RIBSchemaObject(const std::string& class_name, const bool mandatory,
			const unsigned max_objs);
	void addChild(RIBSchemaObject *object);
	const std::string& get_class_name() const;
	unsigned get_max_objs() const;
private:
	std::string class_name_;
	RIBSchemaObject *parent_;
	std::list<RIBSchemaObject*> children_;
	bool mandatory_;
	unsigned max_objs_;
};

//
// EmptyClass (really?)
//
class EmptyClass {};

class EmptyEncoder : public rib::Encoder<EmptyClass> {

public:
	virtual void encode(const EmptyClass &obj, cdap_rib::SerializedObject& serobj){
		(void)serobj;
		(void)obj;
	};
	virtual void decode(const cdap_rib::SerializedObject &serobj,
			EmptyClass& des_obj){
		(void)serobj;
		(void)des_obj;
	};
	std::string get_type() const{
		return "EmptyClass";
	};
};

//fwd decl
class RIBDaemon;


///
/// RIB handle type
///
typedef int64_t rib_handle_t;

//
// RIBDaemon Proxy class
//
class RIBDaemonProxy{

public:

	//-------------------------------------------------------------------//
	//                         Local RIBs                                //
	//-------------------------------------------------------------------//


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
	rib_handle_t createRIB(const cdap_rib::vers_info_t& version);

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
	/// @throws eRIBNotFound, eRIBAlreadyAssociated or Exception on failure
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
	/// @throws eRIBNotFound
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
	/// @param fqn Fully qualified name (position in the tree)
	/// @param obj A pointer (to a pointer) of the object to be added
	///
	/// @ret The instance id of the object created
	/// @throws eRIBNotFound, eObjExists, eObjInvalid, eObjNoParent
	///
	template<typename T>
	int64_t addObjRIB(const rib_handle_t& handle, const std::string& fqn,
							 RIBObj<T>** obj){
		//Recover the non-templatized part
		RIBObj_** obj_ = obj;
		return __addObjRIB(handle, fqn, obj_);
	}

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
	std::string getObjfqn(const rib_handle_t& handle,
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
	///
	/// @throws eRIBNotFound, eObjDoesNotExist
	///
	void removeObjRIB(const rib_handle_t& handle, const int64_t inst_id);


	//-------------------------------------------------------------------//
	//                         RIB Client                                //
	//-------------------------------------------------------------------//

	///
	/// Establish a CDAP connection to a remote RIB
	///
	/// @param ver RIB version
	/// @param src Application source information
	/// @param dst Application dst information
	/// @param auth CDAP Authentication context
	/// @param port_id Flow port id to be used
	/// @ret A CDAP connection handle
	///
	cdap_rib::con_handle_t remote_open_connection(
			const cdap_rib::vers_info_t &ver,
			const cdap_rib::src_info_t &src,
			const cdap_rib::dest_info_t &dest,
			const cdap_rib::auth_info &auth, int port_id);

	///
	/// Close a CDAP connection to a remote RIB
	///
	/// @ret success/failure
	///
	int remote_close_connection(unsigned int port);

	///
	/// Perform a create operation over an object of the remote RIB
	///
	/// @ret success/failure
	///
	int remote_create(unsigned int port,
				  const cdap_rib::obj_info_t &obj,
				  const cdap_rib::flags_t &flags,
				  const cdap_rib::filt_info_t &filt);

	///
	/// Perform a delete operation over an object of the remote RIB
	///
	/// @ret success/failure
	///
	int remote_delete(unsigned int port,
				  const cdap_rib::obj_info_t &obj,
				  const cdap_rib::flags_t &flags,
				  const cdap_rib::filt_info_t &filt);

	///
	/// Perform a read operation over an object of the remote RIB
	///
	/// @ret success/failure
	///
	int remote_read(unsigned int port,
				const cdap_rib::obj_info_t &obj,
				const cdap_rib::flags_t &flags,
				const cdap_rib::filt_info_t &filt);
	///
	/// Perform a cancel read operation over an object of the remote RIB
	///
	/// @ret success/failure
	///
	int remote_cancel_read(unsigned int port,
				       const cdap_rib::flags_t &flags,
				       int invoke_id);

	///
	/// Perform a write operation over an object of the remote RIB
	///
	/// @ret success/failure
	///
	int remote_write(unsigned int port,
				 const cdap_rib::obj_info_t &obj,
				 const cdap_rib::flags_t &flags,
				 const cdap_rib::filt_info_t &filt);

	///
	/// Perform a start operation over an object of the remote RIB
	///
	/// @ret success/failure
	///
	int remote_start(unsigned int port,
				 const cdap_rib::obj_info_t &obj,
				 const cdap_rib::flags_t &flags,
				 const cdap_rib::filt_info_t &filt);

	///
	/// Perform a stop operation over an object of the remote RIB
	///
	/// @ret success/failure
	///
	int remote_stop(unsigned int port,
				const cdap_rib::obj_info_t &obj,
				const cdap_rib::flags_t &flags,
				const cdap_rib::filt_info_t &filt);

private:

	///@internal
	int64_t __addObjRIB(const rib_handle_t& h, const std::string& fqn,
								 RIBObj_** o);

	//Constructor
	RIBDaemonProxy(RIBDaemon* ribd_);

	friend RIBDaemonProxy* RIBDaemonProxyFactory();

	RIBDaemon* ribd;
};


} //namespace rib
} //namespace rina
#endif /* RIB_PROVIDER_H_ */
